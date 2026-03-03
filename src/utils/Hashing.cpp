#include "utils/Hashing.hpp"

#include <array>
#include <iomanip>
#include <random>
#include <sstream>
#include <vector>

namespace libraflow::utils {

namespace {

constexpr std::array<unsigned int, 64> kValues{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

inline unsigned int rotr(unsigned int value, int bits) {
  return (value >> bits) | (value << (32 - bits));
}

}  // namespace

std::string PasswordHasher::random_salt() {
  std::random_device device;
  std::mt19937 generator(device());
  std::uniform_int_distribution<int> distribution(0, 255);

  std::ostringstream output;
  for (int i = 0; i < 16; ++i) {
    output << std::hex << std::setw(2) << std::setfill('0') << distribution(generator);
  }
  return output.str();
}

std::string PasswordHasher::sha256_hex(const std::string& input) {
  std::vector<unsigned char> message(input.begin(), input.end());
  const std::uint64_t bit_length = static_cast<std::uint64_t>(message.size()) * 8U;
  message.push_back(0x80);
  while ((message.size() % 64) != 56) {
    message.push_back(0x00);
  }
  for (int shift = 56; shift >= 0; shift -= 8) {
    message.push_back(static_cast<unsigned char>((bit_length >> shift) & 0xFFU));
  }

  unsigned int h0 = 0x6a09e667U;
  unsigned int h1 = 0xbb67ae85U;
  unsigned int h2 = 0x3c6ef372U;
  unsigned int h3 = 0xa54ff53aU;
  unsigned int h4 = 0x510e527fU;
  unsigned int h5 = 0x9b05688cU;
  unsigned int h6 = 0x1f83d9abU;
  unsigned int h7 = 0x5be0cd19U;

  for (std::size_t chunk = 0; chunk < message.size(); chunk += 64) {
    std::array<unsigned int, 64> words{};
    for (int i = 0; i < 16; ++i) {
      const std::size_t offset = chunk + (i * 4);
      words[i] = (static_cast<unsigned int>(message[offset]) << 24) |
                 (static_cast<unsigned int>(message[offset + 1]) << 16) |
                 (static_cast<unsigned int>(message[offset + 2]) << 8) |
                 static_cast<unsigned int>(message[offset + 3]);
    }
    for (int i = 16; i < 64; ++i) {
      const unsigned int s0 = rotr(words[i - 15], 7) ^ rotr(words[i - 15], 18) ^ (words[i - 15] >> 3);
      const unsigned int s1 = rotr(words[i - 2], 17) ^ rotr(words[i - 2], 19) ^ (words[i - 2] >> 10);
      words[i] = words[i - 16] + s0 + words[i - 7] + s1;
    }

    unsigned int a = h0;
    unsigned int b = h1;
    unsigned int c = h2;
    unsigned int d = h3;
    unsigned int e = h4;
    unsigned int f = h5;
    unsigned int g = h6;
    unsigned int h = h7;

    for (int i = 0; i < 64; ++i) {
      const unsigned int s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
      const unsigned int ch = (e & f) ^ ((~e) & g);
      const unsigned int temp1 = h + s1 + ch + kValues[i] + words[i];
      const unsigned int s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
      const unsigned int maj = (a & b) ^ (a & c) ^ (b & c);
      const unsigned int temp2 = s0 + maj;

      h = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
    }

    h0 += a;
    h1 += b;
    h2 += c;
    h3 += d;
    h4 += e;
    h5 += f;
    h6 += g;
    h7 += h;
  }

  std::ostringstream output;
  output << std::hex << std::setfill('0')
         << std::setw(8) << h0 << std::setw(8) << h1 << std::setw(8) << h2 << std::setw(8) << h3
         << std::setw(8) << h4 << std::setw(8) << h5 << std::setw(8) << h6 << std::setw(8) << h7;
  return output.str();
}

std::string PasswordHasher::hash_password(const std::string& password) {
  const auto salt = random_salt();
  return salt + "$" + sha256_hex(salt + password);
}

bool PasswordHasher::verify_password(const std::string& password, const std::string& stored) {
  const auto separator = stored.find('$');
  if (separator == std::string::npos) {
    return false;
  }
  const std::string salt = stored.substr(0, separator);
  const std::string digest = stored.substr(separator + 1);
  return sha256_hex(salt + password) == digest;
}

}  // namespace libraflow::utils
