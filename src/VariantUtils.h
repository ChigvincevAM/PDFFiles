#include "Component.h"

class VariantUtils {
public:
    static std::string GetString(const variant_t& var);

    static double GetDouble(const variant_t& var);

    static bool GetBool(const variant_t& var);
};