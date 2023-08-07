#include <spdlog/fmt/fmt.h>

#include <optional>
#include <string>

namespace dft::utils {

// Helper to format optional values for the config reports
template <typename T>
std::string FormatForReport(const std::optional<T>& opt)
{
  if (opt.has_value()) {
    return fmt::format("{}", opt.value());
  }
  return "Undefined";
}

}  // namespace dft::utils
