#pragma once

#include <drogon/HttpController.h>

class UploadController : public drogon::HttpController<UploadController> {
public:
  METHOD_LIST_BEGIN

  ADD_METHOD_TO(UploadController::upload, "/api/upload", drogon::Post,
                "RateLimitFilter", "AuthFilter");
  ADD_METHOD_TO(UploadController::previewMedia, "/api/media/preview", drogon::Get);

  METHOD_LIST_END

  void upload(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void previewMedia(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
