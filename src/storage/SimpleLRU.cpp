#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


void SimpleLRU::delete_lru(){
  lru_node *lru = _lru_head.get();
  free_space += (lru -> key.size() + lru -> value.size());
  _lru_index.erase(lru -> key);
  if(_lru_head -> next){
    _lru_head = std::move(_lru_head -> next);
  }
  else{
    _lru_head.reset();
  }
}

void SimpleLRU::update(lru_node &node){
  if (_lru_tail->key != node.key){
    //el
    if(_lru_head->key != node.key){
      _lru_tail -> next = std::move(node.prev->next);
      node.next -> prev = node.prev;
      node.prev -> next = std::move(node.next);
      _lru_tail = &node;
    }
    //head
    else{
      _lru_tail -> next = std::move(_lru_head);
      _lru_head = std::move(node.next);
      node.prev = _lru_tail;
      _lru_tail = &node;
      _lru_head->prev = nullptr;
    }
  }
}

void SimpleLRU::add_elem_back(const std::string &key, const std::string &value){

  std :: unique_ptr<lru_node> new_node(new lru_node(key, value));

  if(!_lru_head){
    _lru_head = std::move(new_node);
    _lru_tail = _lru_head.get();
  }
  else{
    _lru_tail -> next = std::move(new_node);
    _lru_tail -> next -> prev = _lru_tail;
    _lru_tail = _lru_tail -> next.get();
  }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
  if (key.size() + value.size() > _max_size){ return false; }

  auto iter = _lru_index.find(key);

  if(iter != _lru_index.end()){
    update(iter->second.get());
    //here ^^^ we move our pair to tail

    std::string &val = iter -> second.get().value;
    free_space += val.size();

    while(free_space < value.size()){
      delete_lru();
    }
    val = value;
    free_space -= value.size();

    return true;
  }

  while(free_space < key.size() + value.size()){
    delete_lru();
  }

  free_space -= (key.size() + value.size());
  add_elem_back(key, value);

  _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(_lru_tail->key),
                    std::reference_wrapper<lru_node>(*_lru_tail)));
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {

  auto iter = _lru_index.find(key);

  if (key.size() + value.size() > _max_size){ return false; }

  if(_lru_index.count(key)){ return false; }

  if(iter != _lru_index.end()){ return false; }

  else {
    while(free_space < key.size() + value.size()){
      delete_lru();
    }

    free_space -= (key.size() + value.size());
    add_elem_back(key, value);

    _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(_lru_tail->key),
                      std::reference_wrapper<lru_node>(*_lru_tail)));
    return true;
  }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {

  auto iter = _lru_index.find(key);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key){
  if(!_lru_index.count(key)){ return false; }

  else{
    auto iter = _lru_index.find(key);
    lru_node &node = iter -> second;

    _lru_index.erase(std::reference_wrapper<const std::string>(key));


    //one elem in our list
    if((!node.next) && (!node.prev)){
      _lru_head.reset();
    }
    //head
    else if(!node.prev){
      _lru_head = std::move(_lru_head->next);
    }
    //tail
    else if(!node.next){
      _lru_tail = node.prev;
      _lru_tail -> next.reset();
    }
    //between head and tail
    else{
      node.next -> prev = node.prev;
      node.prev -> next = std::move(node.next);
    }

    free_space += node.key.size() + node.value.size();
  }
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value){
  auto iter = _lru_index.find(key);
  if(iter != _lru_index.end()){
    update(iter->second.get());
    value = iter -> second.get().value;
    return true;
  }
  return false;
}


} // namespace Backend
} // namespace Afina
