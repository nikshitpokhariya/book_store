#pragma once

#include "models/Exchange.h"
#include "repositories/BookRepository.h"
#include "repositories/ExchangeRepository.h"
#include "repositories/UserRepository.h"

#include <string>

class ExchangeService {
public:
  ExchangeService(ExchangeRepository &exchanges, BookRepository &books,
                  UserRepository &users);

  ExchangeRequest createRequest(const std::string &requesterId,
                                const std::string &requestedBookId,
                                const std::string &offeredBookId,
                                const std::string &message);

  PageResult<ExchangeRequest> getSentRequests(const std::string &requesterId,
                                              int page, int limit);

  PageResult<ExchangeRequest>
  getReceivedRequests(const std::string &receiverId, int page, int limit);

  PageResult<ExchangeRequest> getUserHistory(const std::string &userId,
                                            int page, int limit);

  ExchangeRequest getRequestDetails(const std::string &requestId,
                                    const std::string &userId);

  ExchangeRequest acceptExchange(const std::string &requestId,
                                 const std::string &receiverId);

  ExchangeRequest rejectExchange(const std::string &requestId,
                                 const std::string &receiverId,
                                 const std::string &note);

  ExchangeRequest cancelExchange(const std::string &requestId,
                                 const std::string &requesterId,
                                 const std::string &note);

private:
  ExchangeRepository &exchanges_;
  BookRepository &books_;
  UserRepository &users_;
};
