#include "StripedLRU.h"

namespace Afina{
namespace Backend{

StripedLRU::StripedLRU(std::size_t max_size, std::size_t _num_shards) : num_shards(_num_shards){

    if(max_size / num_shards < 1024*1024){
      throw std::runtime_error("StripedLRU: number of shards is more than 4");
    }

    for(size_t i = 0; i < num_shards; ++i){
      shards.emplace_back(new ThreadSafeSimplLRU(max_size / num_shards));
    }
}

bool StripedLRU::Put(const std::string &key, const std::string &value){
  return shards[f_hash(key) % num_shards] -> Put(key,value);
}

bool StripedLRU::PutIfAbsent(const std::string &key, const std::string &value){
  return shards[f_hash(key) % num_shards] -> PutIfAbsent(key,value);
}

bool StripedLRU::Set(const std::string &key, const std::string &value){
  return shards[f_hash(key) % num_shards] -> Set(key,value);
}

bool StripedLRU::Delete(const std::string &key){
  return shards[f_hash(key) % num_shards] -> Delete(key);
}

bool StripedLRU::Get(const std::string &key, std::string &value){
  return shards[f_hash(key) % num_shards] -> Get(key,value);
}

} //Backend
} //Afina
