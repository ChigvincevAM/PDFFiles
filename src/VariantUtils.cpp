#include "VariantUtils.h"

std::string VariantUtils::GetString(const variant_t& var) {
    return std::visit(overloaded{
        [](const std::string& s) { return s; },
        [](auto&&) { return std::string(); }
        }, var);
}

double VariantUtils::GetDouble(const variant_t& var) {
    return std::visit(overloaded{
        [](double d) { return d; },
        [](int i) { return static_cast<double>(i); },
        [](auto&&) { return 0.0; }
        }, var);
}

bool VariantUtils::GetBool(const variant_t& var) {
    return std::visit(overloaded{
        [](bool b) { return b; },
        [](auto&&) { return false; }
        }, var);
}
