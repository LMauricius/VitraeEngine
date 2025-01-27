#pragma once

#include "Vitrae/Pipelines/Shading/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <filesystem>
#include <variant>

namespace Vitrae
{

/**
 * @brief For constants collections and other definitions not assignable via the simple
 * ShaderConstant task
 */
class ShaderHeader : public ShaderTask
{
  protected:
    using ShaderTask::ShaderTask;

  public:
    struct FileLoadParams
    {
        ParamList inputSpecs;
        ParamList outputSpecs;
        ParamList filterSpecs;
        ParamList consumingSpecs;
        std::filesystem::path filepath;
        String friendlyName;
    };
    struct StringParams
    {
        ParamList inputSpecs;
        ParamList outputSpecs;
        ParamList filterSpecs;
        ParamList consumingSpecs;
        String snippet;
        String friendlyName;
    };
};

struct ShaderHeaderKeeperSeed
{
    using Asset = ShaderHeader;
    std::variant<ShaderHeader::FileLoadParams, ShaderHeader::StringParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ShaderHeaderKeeper = dynasma::AbstractKeeper<ShaderHeaderKeeperSeed>;

} // namespace Vitrae