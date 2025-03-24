// constants.h
#pragma once

namespace Constants {
    inline constexpr size_t StreamBufferSize = 64 * 1024;
    inline constexpr int ThrottleDelayMs = 100;
    inline constexpr int BandwidthRetryDelay = 50;
}

namespace HttpStatus {
    inline constexpr int Ok = 200;
    inline constexpr int TooManyRequests = 429;
    inline constexpr int InternalServerError = 500;
    inline constexpr int NotFound = 404;
    inline constexpr int BadRequest = 400;
    inline constexpr int ServiceUnavailable = 503;
    // Add more as needed
}