#ifndef AFINA_STORAGE_STRIPED_LRU_H
#define AFINA_STORAGE_STRIPED_LRU_H

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <afina/Storage.h>
#include "ThreadSafeSimpleLRU.h"

namespace Afina {
namespace Backend {

class StripedLRU : public Afina::Storage {
public:
  StripedLRU(std::size_t max_size = 1024*1024*4, std::size_t _num_shards = 4);

  ~StripedLRU(){}

  bool Put(const std::string &key, const std::string &value) override;

  bool PutIfAbsent(const std::string &key, const std::string &value) override;

  bool Set(const std::string &key, const std::string &value) override;

  bool Delete(const std::string &key) override;

  bool Get(const std::string &key, std::string &value) override;

private:
  std::hash<std::string> f_hash;
  std::vector<std::unique_ptr<ThreadSafeSimplLRU>> shards;
  std::size_t num_shards;
};

} // namespace Backend
} // namespace Afina
#endif // AFINA_STORAGE_THREAD_SAFE_STRIPED_LRU_H
