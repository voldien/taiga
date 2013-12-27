/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "base/encoding.h"
#include "base/json.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_item.h"
#include "sync/herro.h"
#include "sync/herro_types.h"
#include "sync/herro_util.h"

namespace sync {
namespace herro {

Service::Service() {
  host_ = L"api.herro.co";

  id_ = kHerro;
  canonical_name_ = L"herro";
  name_ = L"Herro";
}

////////////////////////////////////////////////////////////////////////////////

void Service::BuildRequest(Request& request, HttpRequest& http_request) {
  http_request.host = host_;

  // Herro is supposed to return a JSON response for each and every request,
  // so that's what we expect from it
  http_request.header[L"Accept"] = L"application/json";
  http_request.header[L"Accept-Charset"] = L"utf-8";

  if (RequestNeedsAuthentication(request.type)) {
    // Herro uses an API token instead of password. The token has to be manually
    // generated by the user from their dashboard.
    // TODO: Make sure username and token are available
    http_request.header[L"Authorization"] = L"Basic " +
        Base64Encode(request.data[canonical_name_ + L"-username"] + L":" +
                     request.data[canonical_name_ + L"-apitoken"]);
  }

  switch (request.type) {
    case kGetLibraryEntries:
      // Not sure Herro supports gzip compression, but let's put this here anyway
      // TODO: Make sure username is available
      http_request.header[L"Accept-Encoding"] = L"gzip";
      break;
  }

  switch (request.type) {
    BUILD_HTTP_REQUEST(kAddLibraryEntry, AddLibraryEntry);
    BUILD_HTTP_REQUEST(kAuthenticateUser, AuthenticateUser);
    BUILD_HTTP_REQUEST(kDeleteLibraryEntry, DeleteLibraryEntry);
    BUILD_HTTP_REQUEST(kGetLibraryEntries, GetLibraryEntries);
    BUILD_HTTP_REQUEST(kGetMetadataById, GetMetadataById);
    BUILD_HTTP_REQUEST(kSearchTitle, SearchTitle);
    BUILD_HTTP_REQUEST(kUpdateLibraryEntry, UpdateLibraryEntry);
  }
}

void Service::HandleResponse(Response& response, HttpResponse& http_response) {
  if (RequestSucceeded(response, http_response)) {
    switch (response.type) {
      HANDLE_HTTP_RESPONSE(kAddLibraryEntry, AddLibraryEntry);
      HANDLE_HTTP_RESPONSE(kAuthenticateUser, AuthenticateUser);
      HANDLE_HTTP_RESPONSE(kDeleteLibraryEntry, DeleteLibraryEntry);
      HANDLE_HTTP_RESPONSE(kGetLibraryEntries, GetLibraryEntries);
      HANDLE_HTTP_RESPONSE(kGetMetadataById, GetMetadataById);
      HANDLE_HTTP_RESPONSE(kSearchTitle, SearchTitle);
      HANDLE_HTTP_RESPONSE(kUpdateLibraryEntry, UpdateLibraryEntry);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Request builders

void Service::AuthenticateUser(Request& request, HttpRequest& http_request) {
  // Not available
}

void Service::GetLibraryEntries(Request& request, HttpRequest& http_request) {
  http_request.path =
      L"/list/anime/" + request.data[canonical_name_ + L"-username"];
}

void Service::GetMetadataById(Request& request, HttpRequest& http_request) {
  http_request.path = L"/anime/" + request.data[canonical_name_ + L"-id"];

  if (request.data.count(L"characters"))
    http_request.path += L"?characters=true";
}

void Service::SearchTitle(Request& request, HttpRequest& http_request) {
  http_request.path = L"/search/anime/" + request.data[L"title"];
}

void Service::AddLibraryEntry(Request& request, HttpRequest& http_request) {
  request.data[L"action"] = L"add";
  UpdateLibraryEntry(request, http_request);
}

void Service::DeleteLibraryEntry(Request& request, HttpRequest& http_request) {
  request.data[L"action"] = L"delete";
  UpdateLibraryEntry(request, http_request);
}

void Service::UpdateLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.method = L"POST";
  http_request.header[L"Content-Type"] = L"application/json";

  if (!request.data.count(L"action"))
    request.data[L"action"] = L"update";

  http_request.path = L"/list/anime/" + request.data[L"action"];

  Json::Value root;
  root["_id"] = WstrToStr(request.data[canonical_name_ + L"-id"]);
  if (request.data.count(L"status"))
    root["status"] = WstrToStr(ToWstr(TranslateMyStatusTo(
        ToInt(request.data[L"status"]))));
  if (request.data.count(L"episode"))
    root["progress"] = WstrToStr(request.data[L"episode"]);
  if (request.data.count(L"score"))
    root["score"] = WstrToStr(request.data[L"score"]);
#ifdef _DEBUG
  Json::StyledWriter writer;
#else
  Json::FastWriter writer;
#endif
  http_request.body = StrToWstr(writer.write(root));
}

////////////////////////////////////////////////////////////////////////////////
// Response handlers

void Service::AuthenticateUser(Response& response, HttpResponse& http_response) {
  // Not available
}

void Service::GetLibraryEntries(Response& response, HttpResponse& http_response) {
  Json::Value root;
  Json::Reader reader;
  bool parsed = reader.parse(WstrToStr(http_response.body), root);

  if (!parsed) {
    response.data[L"error"] = L"Could not parse the list";
    return;
  }

  // Available data:
  // - _id
  // - series_title
  // - list_score (0-10)
  // - list_status (1-5)
  // - list_progress (0-n)
  // - series_total
  // - series_type (in string form)
  // - series_status (in string form)
  for (size_t i = 0; i < root.size(); i++) {
    auto& value = root[i];
    ::anime::Item anime_item;
    anime_item.SetId(StrToWstr(value["_id"].asString()), this->id());
    anime_item.last_modified = time(nullptr);  // current time

    anime_item.SetTitle(StrToWstr(value["series_title"].asString()));
    anime_item.SetEpisodeCount(value["series_total"].asInt());
    anime_item.SetType(TranslateSeriesTypeFrom(StrToWstr(value["series_type"].asString())));
    anime_item.SetAiringStatus(TranslateSeriesStatusFrom(StrToWstr(value["series_status"].asString())));

    anime_item.AddtoUserList();
    anime_item.SetMyScore(value["list_score"].asInt());
    anime_item.SetMyStatus(TranslateMyStatusFrom(value["list_status"].asInt()));
    anime_item.SetMyLastWatchedEpisode(value["list_progress"].asInt());

    AnimeDatabase.UpdateItem(anime_item);
  }
}

void Service::GetMetadataById(Response& response, HttpResponse& http_response) {
  Json::Value root;
  Json::Reader reader;
  bool parsed = reader.parse(WstrToStr(http_response.body), root);

  if (!parsed) {
    response.data[L"error"] = L"Could not parse the anime object";
    return;
  }

  // Available data:
  // - title
  // - slug
  // - image_url
  // - metadata
  //   - title_aka (array)
  //   - title_english (array)
  //   - series_status (in string form)
  //   - series_type (in string form)
  //   - series_start
  //   - series_end
  //   - plot
  //   - genres (array)
  //   - episodes_total
  //   - episodes_length

  ::anime::Item anime_item;
  // TODO: Set ID

  anime_item.SetTitle(StrToWstr(root["title"].asString()));
  anime_item.SetImageUrl(StrToWstr(root["image_url"].asString()));

  auto& metadata = root["metadata"];

  std::vector<std::wstring> title_aka;
  if (JsonReadArray(metadata, "title_aka", title_aka))
    anime_item.SetSynonyms(title_aka);

  std::vector<std::wstring> english;
  if (JsonReadArray(metadata, "english", english))
    anime_item.SetEnglishTitle(english.front());  // TODO

  anime_item.SetAiringStatus(TranslateSeriesStatusFrom(StrToWstr(metadata["series_status"].asString())));
  anime_item.SetType(TranslateSeriesTypeFrom(StrToWstr(metadata["series_type"].asString())));
  anime_item.SetDateStart(TranslateDateFrom(StrToWstr(metadata["series_start"].asString())));
  anime_item.SetDateEnd(TranslateDateFrom(StrToWstr(metadata["series_end"].asString())));
  anime_item.SetSynopsis(StrToWstr(metadata["plot"].asString()));

  std::vector<std::wstring> genres;
  if (JsonReadArray(metadata, "genres", genres))
    anime_item.SetGenres(genres);

  anime_item.SetEpisodeCount(metadata["episodes_total"].asInt());
  anime_item.SetEpisodeLength(metadata["episodes_length"].asInt());

  // TODO: Update database
//AnimeDatabase.UpdateItem(anime_item);
}

void Service::SearchTitle(Response& response, HttpResponse& http_response) {
  Json::Value root;
  Json::Reader reader;
  bool parsed = reader.parse(WstrToStr(http_response.body), root);

  if (!parsed) {
    response.data[L"error"] = L"Could not parse search results";
    return;
  }

  // Available data:
  // - _id
  // - title
  // - slug
  // - image_url
  // - metadata
  //   - title_aka (array)
  //   - title_english (array)
  //   - series_status (in string form)
  //   - series_type (in string form)
  //   - series_start
  //   - series_end
  //   - plot

  for (size_t i = 0; i < root.size(); i++) {
    auto& value = root[i];
    ::anime::Item anime_item;
    anime_item.SetId(StrToWstr(value["_id"].asString()), this->id());
    anime_item.SetTitle(StrToWstr(value["title"].asString()));
    anime_item.SetImageUrl(StrToWstr(value["image_url"].asString()));

    auto& metadata = value["metadata"];

    std::vector<std::wstring> title_aka;
    if (JsonReadArray(metadata, "title_aka", title_aka))
      anime_item.SetSynonyms(title_aka);

    std::vector<std::wstring> english;
    if (JsonReadArray(metadata, "english", english))
      anime_item.SetEnglishTitle(english.front());  // TODO

    anime_item.SetAiringStatus(TranslateSeriesStatusFrom(StrToWstr(metadata["series_status"].asString())));
    anime_item.SetType(TranslateSeriesTypeFrom(StrToWstr(metadata["series_type"].asString())));
    anime_item.SetDateStart(TranslateDateFrom(StrToWstr(metadata["series_start"].asString())));
    anime_item.SetDateEnd(TranslateDateFrom(StrToWstr(metadata["series_end"].asString())));
    anime_item.SetSynopsis(StrToWstr(metadata["plot"].asString()));

    int anime_id = AnimeDatabase.UpdateItem(anime_item);

    // We return a list of IDs so that we can display the results afterwards
    AppendString(response.data[L"ids"], ToWstr(anime_id), L",");
  }
}

void Service::AddLibraryEntry(Response& response, HttpResponse& http_response) {
  // Nothing to do here
}

void Service::DeleteLibraryEntry(Response& response, HttpResponse& http_response) {
  // Nothing to do here
}

void Service::UpdateLibraryEntry(Response& response, HttpResponse& http_response) {
  // Nothing to do here
}

////////////////////////////////////////////////////////////////////////////////

bool Service::RequestNeedsAuthentication(RequestType request_type) const {
  switch (request_type) {
    case kAddLibraryEntry:
    case kAuthenticateUser:
    case kDeleteLibraryEntry:
    case kUpdateLibraryEntry:
      return true;
  }

  return false;
}

bool Service::RequestSucceeded(Response& response,
                               const HttpResponse& http_response) {
  switch (http_response.code) {
    // OK
    case 200:
      return true;

    // Error
    case 500: {
      Json::Value root;
      Json::Reader reader;
      bool parsed = reader.parse(WstrToStr(http_response.body), root);
      if (parsed) {
        response.data[L"error"] = StrToWstr(root["response"].asString());
      } else {
        response.data[L"error"] = L"Unknown error";
      }
      return false;
    }

    default:
      response.data[L"error"] =
          name() + L" returned an unknown response (" +
          ToWstr(static_cast<int>(http_response.code)) + L")";
      return false;
  }
}

}  // namespace herro
}  // namespace sync