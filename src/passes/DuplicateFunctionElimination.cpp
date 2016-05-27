/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Removes duplicate functions. That can happen due to C++ templates,
// and also due to types being different at the source level, but
// identical when finally lowered into concrete wasm code.
//

#include <wasm.h>
#include <pass.h>
#include <ast_utils.h>

namespace wasm {

struct FunctionHasher : public PostWalker<MergeBlocks, Visitor<MergeBlocks>> {
  bool isFunctionParallel() { return true; }

  FunctionHasher* create() override {
    auto* ret = new FunctionHasher;
    ret->setOutput(output);
    return ret;
  }

  void setOutput(std::map<Function, size_t>* output_) {
    output = output_;
  }

  void walk(Expression*& root) {
    assert(digest == 0);
    auto* func = getFunction();
    note(func->getNumParams());
    for (auto type : func->params) note(type);
    note(func->getNumVars());
    for (auto type : func->vars) note(type);
    note(result);
    note(type.is() ? type : 0);
    note(ExpressionHasher::hash(root));
    output->at(func) = digest;
  }

private:
  std::map<Function, size_t>* output;
  size_t digest = 0;

  void note(size_t hash) {
    *output = std::hash<std::pair<size_t, size_t>>(std::pair<size_t, size_t>(*output, hash));
  }
};

struct DuplicateFunctionElimination : public Pass {
  std::map<Function, size_t> hashes;

  void run(PassRunner* runner, Module* module) override {
    // TODO: multiple passes
    // Hash all the functions
    hashes.reserve(module->functions.size());
    for (auto& func : module->functions) {
      hashes[func.get()] = 0; // ensure an entry for each function - we must not modify the map shape in parallel, just the values
    }
    FunctionHasher hasher;
    hasher.setOutput(&hashes);
    hasher.startWalk(module);
    // Find hash-equal groups
    std::map<size_t, std::vector<Function*>> hashGroups;
    for (auto& func : module->functions) {
      hashGroups[hashes[func.get()]].push_back(func.get());
    }
    // Find actually equal functions
    for (auto& pair : hashGroups) {
      auto& group = pair.second;
    }
  }
};

static RegisterPass<DuplicateFunctionElimination> registerPass("dfe", "removes duplicate functions");

} // namespace wasm

