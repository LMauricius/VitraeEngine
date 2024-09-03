#pragma once

#include "Probe.hpp"

namespace GI
{

extern const char *const GLSL_PROBE_GEN_SNIPPET;

void generateProbeList(std::vector<H_ProbeDefinition> &probes, glm::vec3 worldCenter,
                       glm::vec3 worldSize, float minProbeSize);
} // namespace GI