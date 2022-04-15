/**
 * Fibonacci Queue
 * Copyright (c) 2014, Emmanuel Benazera beniz@droidnik.fr, All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

/**
 * This is basically a Fibonacci heap with an added fast store for retrieving
 * nodes, and decrease their key as needed. Useful for search algorithms (e.g.
 * Dijstra, heuristic, ...).
 */

#ifndef FIBOQUEUE_H
#define FIBOQUEUE_H

#include "fiboheap.h"
#include <unordered_map>
#include <algorithm>

template<class T, class Comp = std::less<T>>
class FibQueue : public FibHeap<T, Comp>
{
 public:
  using Heap = FibHeap<T, Comp>;
  using Node = typename Heap::FibNode;
  using KeyNodeIter = typename std::unordered_map<T, Node*>::iterator;

  FibQueue()
    : Heap()
    {
    }

  FibQueue(Comp comp)
      : Heap(comp)
  {
  }

  ~FibQueue()
    {
    }

  void decrease_key(int i_pos, T old, T k)
  {
    if (fstore.count(i_pos) == 0) {
      std::cout << "node " << i_pos << " not found!" << std::endl;
      return;
    }
    auto node = fstore.at(i_pos);
    Heap::decrease_key(node, std::move(k));
  }

  Node* push(T k, int string_pos)
  {
    Node *x = Heap::push(std::move(k),string_pos);
    fstore[string_pos] = x;
    return x;
  }

  int pop()
  {
    if (Heap::empty())
      return -1;
    Node *x = Heap::extract_min();
    if (!x)
      return -1; // should not happen.
    int string_pos = x->payload;
    fstore.erase(string_pos);
    delete x;
    return string_pos;
  }

  void clear() {
    Heap::clear();
    fstore.clear();
  }

  std::unordered_map<int, Node*> fstore;
};

#endif
