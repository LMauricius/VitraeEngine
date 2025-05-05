#pragma once

#include <any>
#include <vector>

namespace Vitrae {
    
    class PipelineMemory {
    public:
      PipelineMemory();
      virtual ~PipelineMemory() = default;

      template <typename T, typename... Args> auto &createNext(Args &&...args)
      {
          return createNextAny().emplace<T>(std::forward<Args>(args)...);
      }

      template<typename T>
      T &next() {
          return std::any_cast<T&>(nextAny());
      }

    protected:
        std::size_t m_nextAnyIndex;
        std::vector<std::any> m_anyPool;

        std::any &createNextAny();
        std::any &nextAny();
    };

    class RestartablePipelineMemory : public PipelineMemory {
    public:
      RestartablePipelineMemory();
      ~RestartablePipelineMemory() = default;

      void restart();
      void clear();
    };
}