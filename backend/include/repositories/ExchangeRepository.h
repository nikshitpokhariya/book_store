#pragma once

#include "models/Book.h"
#include "models/Exchange.h"
#include <mongocxx/client_session.hpp>

#include <optional>
#include <string>
#include <vector>

class ExchangeRepository {
public:
  std::string create(const ExchangeRequest &req);

  std::optional<ExchangeRequest> findById(const std::string &id);
  std::optional<ExchangeRequest> findById(mongocxx::client_session &session,
                                          const std::string &id);

  PageResult<ExchangeRequest> findByRequester(const std::string &requesterId,
                                              int page, int limit);

  PageResult<ExchangeRequest> findByReceiver(const std::string &receiverId,
                                             int page, int limit);

  PageResult<ExchangeRequest> findUserHistory(const std::string &userId,
                                              int page, int limit);

  bool updateStatus(const std::string &id, const std::string &newStatus,
                    const std::string &note, bool setCompletedAt = false,
                    const std::string &expectedStatus = "");

  bool updateStatus(mongocxx::client_session &session,
                    const std::string &id, const std::string &newStatus,
                    const std::string &note, bool setCompletedAt = false,
                    const std::string &expectedStatus = "");
};
