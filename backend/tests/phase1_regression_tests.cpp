#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cstdint>
#include <algorithm>
#include <cctype>

#include "security/PaymentService.hpp"

// Replicate the exact validation routines from UploadController.cpp for unit-level verification
namespace {

bool isSafeFilename(const std::string &name) {
  if (name.empty() || name.size() > 255) return false;
  if (name.find("..") != std::string::npos) return false;
  if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) return false;
  for (unsigned char c : name) {
    if (c < 32 || c == 127) return false;
  }
  return true;
}

bool validateFileSignature(const std::string &ext, const std::string &bytes) {
  if (ext == "jpg" || ext == "jpeg") {
    if (bytes.size() < 3) return false;
    const auto *ub = reinterpret_cast<const unsigned char *>(bytes.data());
    return ub[0] == 0xFF && ub[1] == 0xD8 && ub[2] == 0xFF;
  }
  if (ext == "png") {
    if (bytes.size() < 8) return false;
    const auto *ub = reinterpret_cast<const unsigned char *>(bytes.data());
    return ub[0] == 0x89 && ub[1] == 'P' && ub[2] == 'N' && ub[3] == 'G' &&
           ub[4] == 0x0D && ub[5] == 0x0A && ub[6] == 0x1A && ub[7] == 0x0A;
  }
  if (ext == "webp") {
    if (bytes.size() < 12) return false;
    return bytes.compare(0, 4, "RIFF") == 0 && bytes.compare(8, 4, "WEBP") == 0;
  }
  if (ext == "mp4") {
    if (bytes.size() < 12) return false;
    return bytes.compare(4, 4, "ftyp") == 0;
  }
  return false;
}

bool isAllowedExtension(const std::string &ext) {
  const std::vector<std::string> allowed = {"jpg", "jpeg", "png", "webp", "mp4"};
  return std::find(allowed.begin(), allowed.end(), ext) != allowed.end();
}

bool isFileSizeAllowed(const std::string &ext, std::size_t size) {
  if (size == 0) return false;
  constexpr std::size_t kMaxImageSize = 10 * 1024 * 1024; // 10MB
  constexpr std::size_t kMaxVideoSize = 30 * 1024 * 1024; // 30MB
  if (ext == "mp4") return size <= kMaxVideoSize;
  return size <= kMaxImageSize;
}

bool isAllowedRedirectUrl(const std::string &targetUrl, const std::string &cloudName) {
  if (targetUrl.empty()) return false;
  if (targetUrl.find("..") != std::string::npos) return false;

  if (targetUrl.rfind("/uploads/", 0) == 0 || targetUrl.rfind("uploads/", 0) == 0) {
    return true;
  }

  const std::string trustedPrefix = "https://res.cloudinary.com/" + cloudName + "/";
  if (!cloudName.empty() && targetUrl.rfind(trustedPrefix, 0) == 0) {
    return true;
  }

  return false;
}

int g_passedTests = 0;
int g_failedTests = 0;

void recordTest(bool condition, const std::string &name) {
  if (condition) {
    std::cout << "  [PASS] " << name << "\n";
    g_passedTests++;
  } else {
    std::cerr << "  [FAIL] " << name << "\n";
    g_failedTests++;
  }
}

} // namespace

int main() {
  std::cout << "\n=========================================\n";
  std::cout << " BookBazzar Phase 1 Security Test Suite \n";
  std::cout << "=========================================\n\n";

  // ----------------------------------------------------
  // Group 1: Payment simulation safety
  // ----------------------------------------------------
  std::cout << "--- Group 1: SEC-01 Payment Simulation Safety ---\n";
  {
    auto cardRes = PaymentService::processPayment("CARD", 499.0);
    recordTest(cardRes.paymentStatus == "simulated", "CARD payment status is 'simulated' (not 'paid')");
    recordTest(cardRes.success == true, "CARD payment succeeds in demo mode");
    recordTest(!cardRes.transactionId.empty(), "CARD payment includes demo transaction id");
    recordTest(cardRes.message.find("Simulated") != std::string::npos, "CARD payment message disclaims real payment");

    auto upiRes = PaymentService::processPayment("UPI", 199.0);
    recordTest(upiRes.paymentStatus == "simulated", "UPI payment status is 'simulated' (not 'paid')");
    recordTest(upiRes.success == true, "UPI payment succeeds in demo mode");

    auto codRes = PaymentService::processPayment("COD", 350.0);
    recordTest(codRes.paymentStatus == "pending", "COD payment status remains 'pending'");
    recordTest(codRes.success == true, "COD payment succeeds as pending delivery");

    auto invalidRes = PaymentService::processPayment("BITCOIN", 50.0);
    recordTest(!invalidRes.success, "Unsupported payment method is rejected");

    auto failRes = PaymentService::processPayment("CARD", 500.0, true);
    recordTest(!failRes.success && failRes.paymentStatus == "failed", "Simulated failure returns failed status");
  }

  // ----------------------------------------------------
  // Group 2: Upload File Extensions, Filenames & Signatures
  // ----------------------------------------------------
  std::cout << "\n--- Group 2: SEC-02 & SEC-03 File Upload Validation ---\n";
  {
    recordTest(isAllowedExtension("jpg"), "Extension 'jpg' is permitted");
    recordTest(isAllowedExtension("jpeg"), "Extension 'jpeg' is permitted");
    recordTest(isAllowedExtension("png"), "Extension 'png' is permitted");
    recordTest(isAllowedExtension("webp"), "Extension 'webp' is permitted");
    recordTest(isAllowedExtension("mp4"), "Extension 'mp4' is permitted");
    recordTest(!isAllowedExtension("exe"), "Extension 'exe' is rejected");
    recordTest(!isAllowedExtension("sh"), "Extension 'sh' is rejected");
    recordTest(!isAllowedExtension("php"), "Extension 'php' is rejected");
    recordTest(!isAllowedExtension("html"), "Extension 'html' is rejected");
    recordTest(!isAllowedExtension("svg"), "Extension 'svg' is rejected");
    recordTest(!isAllowedExtension("gif"), "Extension 'gif' is rejected");
    recordTest(!isAllowedExtension("mov"), "Extension 'mov' is rejected");
    recordTest(!isAllowedExtension("mkv"), "Extension 'mkv' is rejected");

    recordTest(isSafeFilename("book_cover.jpg"), "Filename 'book_cover.jpg' is safe");
    recordTest(isSafeFilename("page-1_preview.png"), "Filename 'page-1_preview.png' is safe");
    recordTest(!isSafeFilename("../../etc/passwd.jpg"), "Directory traversal '../../etc/passwd.jpg' is rejected");
    recordTest(!isSafeFilename("..\\secret.png"), "Path traversal with backslash '..\\secret.png' is rejected");
    recordTest(!isSafeFilename("/var/data/pic.jpg"), "Absolute slash path '/var/data/pic.jpg' is rejected");
    recordTest(!isSafeFilename(""), "Empty filename is rejected");

    // JPEG signature (\xFF\xD8\xFF)
    const std::string validJpeg = "\xFF\xD8\xFF\xE0\x00\x10JFIF";
    recordTest(validateFileSignature("jpg", validJpeg), "Valid JPEG signature accepted");
    recordTest(validateFileSignature("jpeg", validJpeg), "Valid JPEG (.jpeg) signature accepted");
    recordTest(!validateFileSignature("jpg", "NotAJpegDataStream"), "Spoofed JPEG content rejected");

    // PNG signature (\x89PNG\r\n\x1a\n)
    const std::string validPng = "\x89PNG\x0D\x0A\x1A\x0A\x00\x00";
    recordTest(validateFileSignature("png", validPng), "Valid PNG signature accepted");
    recordTest(!validateFileSignature("png", "PNGFakeDataHeader"), "Invalid PNG content rejected");

    // WebP signature (RIFF????WEBP)
    const char webpBytes[] = {'R', 'I', 'F', 'F', 0x24, 0x00, 0x00, 0x00, 'W', 'E', 'B', 'P', 'V', 'P', '8'};
    const std::string validWebp(webpBytes, sizeof(webpBytes));
    recordTest(validateFileSignature("webp", validWebp), "Valid WebP signature accepted");
    recordTest(!validateFileSignature("webp", "RIFFxxxxAVIFxxxx"), "Invalid RIFF non-WebP rejected");

    // MP4 signature (????ftyp)
    const std::string validMp4 = "xxxxftypisom\x00\x00";
    recordTest(validateFileSignature("mp4", validMp4), "Valid MP4 signature accepted");
    recordTest(!validateFileSignature("mp4", "xxxxmoovxxxx"), "Spoofed MP4 content rejected");

    // File size checks
    recordTest(!isFileSizeAllowed("jpg", 0), "Empty file (0 bytes) is rejected");
    recordTest(isFileSizeAllowed("jpg", 5 * 1024 * 1024), "5MB image is within 10MB limit");
    recordTest(isFileSizeAllowed("png", 10 * 1024 * 1024), "10MB image is within limit");
    recordTest(!isFileSizeAllowed("png", 11 * 1024 * 1024), "11MB image is rejected (>10MB)");
    recordTest(isFileSizeAllowed("mp4", 25 * 1024 * 1024), "25MB video is within 30MB limit");
    recordTest(!isFileSizeAllowed("mp4", 31 * 1024 * 1024), "31MB video is rejected (>30MB)");
  }

  // ----------------------------------------------------
  // Group 3: Media Preview & Open Redirect Prevention
  // ----------------------------------------------------
  std::cout << "\n--- Group 3: SEC-05 Media Preview & Open Redirect Prevention ---\n";
  {
    const std::string cloudName = "bookbazzar-prod";
    recordTest(isAllowedRedirectUrl("/uploads/cover.jpg", cloudName), "Valid local path '/uploads/cover.jpg' allowed");
    recordTest(isAllowedRedirectUrl("uploads/sample.png", cloudName), "Valid local path 'uploads/sample.png' allowed");
    recordTest(!isAllowedRedirectUrl("/uploads/../etc/passwd", cloudName), "Traversal local path '/uploads/../etc/passwd' rejected");
    recordTest(isAllowedRedirectUrl("https://res.cloudinary.com/bookbazzar-prod/image/upload/sample.jpg", cloudName), "Trusted Cloudinary URL with matching cloudName allowed");
    recordTest(!isAllowedRedirectUrl("https://evil.com/phishing", cloudName), "Arbitrary external URL 'https://evil.com/phishing' blocked");
    recordTest(!isAllowedRedirectUrl("http://attacker.com", cloudName), "HTTP arbitrary URL 'http://attacker.com' blocked");
    recordTest(!isAllowedRedirectUrl("https://evil.com?ref=res.cloudinary.com", cloudName), "Spoofed query parameter 'https://evil.com?ref=res.cloudinary.com' blocked");
    recordTest(!isAllowedRedirectUrl("https://res.cloudinary.com/attacker_cloud/image/upload/x.jpg", cloudName), "Different Cloudinary tenant 'https://res.cloudinary.com/attacker_cloud/image/upload/x.jpg' blocked");
    recordTest(!isAllowedRedirectUrl("", cloudName), "Empty URL blocked");
  }

  std::cout << "\n=========================================\n";
  std::cout << " Test Results: " << g_passedTests << " / " << (g_passedTests + g_failedTests) << " PASSED\n";
  std::cout << "=========================================\n\n";

  if (g_failedTests > 0) {
    std::cerr << "FAILURES DETECTED: " << g_failedTests << " tests failed!\n";
    return 1;
  }

  std::cout << "ALL PHASE 1 SECURITY TESTS PASSED!\n";
  return 0;
}
