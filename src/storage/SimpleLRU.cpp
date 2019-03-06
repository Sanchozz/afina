#include "SimpleLRU.h"
#include <iostream>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) { 
    auto found_pair = _lru_index.find(key);
    if (found_pair != _lru_index.end()) {
        _delete_node(&found_pair->second.get());
    }

    int status;
    if ((status = _add_new_node(key, value))) {
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(_lru_tail->key),
                    std::reference_wrapper<lru_node>(*_lru_tail)));
    }
    return status;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    auto found_pair = _lru_index.find(key);
    if (found_pair != _lru_index.end()) {
        return false;
    }

    int status;
    if ((status = _add_new_node(key, value))) {
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(_lru_tail->key),
                    std::reference_wrapper<lru_node>(*_lru_tail)));
    }
    return status;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto found_pair = _lru_index.find(key);
    if (found_pair != _lru_index.end()) {
        _delete_node(&found_pair->second.get());
    } else {
        return false;
    }

    int status;
    if ((status = _add_new_node(key, value))) {
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(_lru_tail->key),
                    std::reference_wrapper<lru_node>(*_lru_tail)));
    }
    return status;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto found_pair = _lru_index.find(key);
    if (found_pair == _lru_index.end()) {
        return false;
    }

    return _delete_node(&found_pair->second.get());
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto found_pair = _lru_index.find(key);
    if (found_pair == _lru_index.end()) {
        return false;
    }

    value = (&found_pair->second.get())->value;
    return true;
}



bool SimpleLRU::_add_new_node(const std::string &key, const std::string &value) {
    if (_max_size < key.size() + value.size()) {
        return false;
    } 

    while (_max_size - _cur_size < key.size() + value.size()) {
        _delete_old_node();
    }

    std::unique_ptr<lru_node> new_node(new lru_node(key, value));
    _cur_size += key.size() + value.size();

    if (_lru_head) {
        new_node->prev = _lru_tail;
        _lru_tail->next.swap(new_node); //check!!!!
        _lru_tail = _lru_tail->next.get();
    } else {
        _lru_head.swap(new_node);
        _lru_tail = _lru_head.get();
    }

    return true;
}

bool SimpleLRU::_delete_old_node() {
    if (!_lru_head) {
        return false;
    }

    _lru_index.erase(_lru_head->key);
    _cur_size -= _lru_head->key.size() + _lru_head->value.size();
    if (_lru_head->next) {
        std::unique_ptr<lru_node> tmp(nullptr);
        tmp.swap(_lru_head->next);
        _lru_head.swap(tmp);
        _lru_head->prev = nullptr;
        tmp.reset(nullptr);
    } else {
        _lru_head.reset();
        _lru_tail = nullptr;
    }
    /*if (_lru_head->next) {
            _lru_head = std::move(_lru_head->next);
            _lru_head->prev = nullptr;
        } else {
            _lru_head.reset(nullptr);
            _lru_tail = nullptr;
    }*/

    return true;
}

bool SimpleLRU::_delete_node(lru_node *node) {
    if (!node) {
        return false;
    }

    _lru_index.erase(_lru_head->key);
    if (!node->prev) {
        _delete_old_node();
    } else {
        _cur_size -= node->key.size() + node->value.size();
        if (node->next) {
            node->prev->next.swap(node->next);
            node->next->prev = node->prev;
            delete node;
        } else {
            lru_node *tmp = _lru_tail;
            _lru_tail = _lru_tail->prev;
            delete tmp;
        }
    }
    return true;
}

} // namespace Backend
} // namespace Afina
