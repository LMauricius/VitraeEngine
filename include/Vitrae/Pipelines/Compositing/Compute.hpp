#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Setup/GpuCompute.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>
#include <vector>

namespace Vitrae
{

class ComposeCompute : public ComposeTask
{
  public:
    struct SetupParams
    {
        ComponentRoot &root;
        std::vector<String> outputTokenNames;
        ParamList iterationOutputSpecs;
        GpuComputeSetupParams computeSetup;
        bool cacheResults;
    };
};

struct ComposeComputeKeeperSeed
{
    using Asset = ComposeCompute;
    std::variant<ComposeCompute::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeComputeKeeper = dynasma::AbstractKeeper<ComposeComputeKeeperSeed>;
} // namespace Vitrae