// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROFILER_MODULE_CACHE_H_
#define BASE_PROFILER_MODULE_CACHE_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/base_export.h"
#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/win/windows_types.h"
#endif

namespace base {

// Supports cached lookup of modules by address, with caching based on module
// address ranges.
//
// Cached lookup is necessary on Mac for performance, due to an inefficient
// dladdr implementation. See https://crrev.com/487092.
//
// Cached lookup is beneficial on Windows to minimize use of the loader
// lock. Note however that the cache retains a handle to looked-up modules for
// its lifetime, which may result in pinning modules in memory that were
// transiently loaded by the OS.
class BASE_EXPORT ModuleCache {
 public:
  // Module represents a binary module (executable or library) and its
  // associated state.
  class BASE_EXPORT Module {
   public:
    Module() = default;
    virtual ~Module() = default;

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    // Gets the base address of the module.
    virtual uintptr_t GetBaseAddress() const = 0;

    // Gets the opaque binary string that uniquely identifies a particular
    // program version with high probability. This is parsed from headers of the
    // loaded module.
    // For binaries generated by GNU tools:
    //   Contents of the .note.gnu.build-id field.
    // On Windows:
    //   GUID + AGE in the debug image headers of a module.
    virtual std::string GetId() const = 0;

    // Gets the debug basename of the module. This is the basename of the PDB
    // file on Windows and the basename of the binary on other platforms.
    virtual FilePath GetDebugBasename() const = 0;

    // Gets the size of the module.
    virtual size_t GetSize() const = 0;

    // True if this is a native module.
    virtual bool IsNative() const = 0;
  };

  // Interface for lazily creating a native module for a given |address|. The
  // provider is registered with RegisterAuxiliaryModuleProvider().
  class AuxiliaryModuleProvider {
   public:
    AuxiliaryModuleProvider() = default;
    AuxiliaryModuleProvider(const AuxiliaryModuleProvider&) = delete;
    AuxiliaryModuleProvider& operator=(const AuxiliaryModuleProvider&) = delete;

    virtual std::unique_ptr<const Module> TryCreateModuleForAddress(
        uintptr_t address) = 0;

   protected:
    ~AuxiliaryModuleProvider() = default;
  };

  ModuleCache();
  ~ModuleCache();

  // Gets the module containing |address| or nullptr if |address| is not within
  // a module. The returned module remains owned by and has the same lifetime as
  // the ModuleCache object.
  const Module* GetModuleForAddress(uintptr_t address);
  std::vector<const Module*> GetModules() const;

  // Updates the set of non-native modules maintained by the
  // ModuleCache. Non-native modules represent regions of non-native executable
  // code such as V8 generated code.
  //
  // Note that non-native modules may be embedded within native modules, as in
  // the case of V8 builtin code compiled within Chrome. In that case
  // GetModuleForAddress() will return the non-native module rather than the
  // native module for the memory region it occupies.
  //
  // Modules in |defunct_modules| are removed from the set of active modules;
  // specifically they no longer participate in the GetModuleForAddress()
  // lookup. They continue to exist for the lifetime of the ModuleCache,
  // however, so that existing references to them remain valid. Modules in
  // |new_modules| are added to the set of active non-native modules. Modules in
  // |new_modules| may not overlap with any non-native Modules already present
  // in ModuleCache, unless those modules are provided in |defunct_modules| in
  // the same call.
  void UpdateNonNativeModules(
      const std::vector<const Module*>& defunct_modules,
      std::vector<std::unique_ptr<const Module>> new_modules);

  // Adds a custom native module to the cache. This is intended to support
  // native modules that require custom handling. In general, native modules
  // will be found and added automatically when invoking GetModuleForAddress().
  // |module| may not overlap with any native Modules already present in
  // ModuleCache.
  void AddCustomNativeModule(std::unique_ptr<const Module> module);

  // Registers a custom module provider for lazily creating native modules. At
  // most one provider can be registered at any time, and the provider must be
  // unregistered before being destroyed. This is intended to support native
  // modules that require custom handling. In general, native modules will be
  // found and added automatically when invoking GetModuleForAddress(). If no
  // module is found, this provider will be used as fallback.
  void RegisterAuxiliaryModuleProvider(
      AuxiliaryModuleProvider* auxiliary_module_provider);

  // Unregisters the custom module provider.
  void UnregisterAuxiliaryModuleProvider(
      AuxiliaryModuleProvider* auxiliary_module_provider);

  // Gets the module containing |address| if one already exists, or nullptr
  // otherwise. The returned module remains owned by and has the same lifetime
  // as the ModuleCache object.
  // NOTE: Only users that create their own modules and need control over native
  // module creation should use this function. Everyone else should use
  // GetModuleForAddress().
  const Module* GetExistingModuleForAddress(uintptr_t address) const;

 private:
  // Heterogenously compares modules by base address, and modules and
  // addresses. The module/address comparison considers the address equivalent
  // to the module if the address is within the extent of the module. Combined
  // with is_transparent this allows modules to be looked up by address in the
  // using containers.
  struct ModuleAndAddressCompare {
    using is_transparent = void;
    bool operator()(const std::unique_ptr<const Module>& m1,
                    const std::unique_ptr<const Module>& m2) const;
    bool operator()(const std::unique_ptr<const Module>& m1,
                    uintptr_t address) const;
    bool operator()(uintptr_t address,
                    const std::unique_ptr<const Module>& m2) const;
  };

  // Creates a Module object for the specified memory address. Returns null if
  // the address does not belong to a module.
  static std::unique_ptr<const Module> CreateModuleForAddress(
      uintptr_t address);

  // Set of native modules sorted by base address. We use set rather than
  // flat_set because the latter type has O(n^2) runtime for adding modules
  // one-at-a-time, which is how modules are added on Windows and Mac.
  std::set<std::unique_ptr<const Module>, ModuleAndAddressCompare>
      native_modules_;

  // Set of non-native modules currently mapped into the address space, sorted
  // by base address. Represented as flat_set because std::set does not support
  // extracting move-only element types prior to C++17's
  // std::set<>::extract(). The non-native module insertion/removal patterns --
  // initial bulk insertion, then infrequent inserts/removals -- should work
  // reasonably well with the flat_set complexity guarantees. Separate from
  // native_modules_ to support preferential lookup of non-native modules
  // embedded in native modules; see comment on UpdateNonNativeModules().
  base::flat_set<std::unique_ptr<const Module>, ModuleAndAddressCompare>
      non_native_modules_;

  // Unsorted vector of inactive non-native modules. Inactive modules are no
  // longer mapped in the address space and don't participate in address lookup,
  // but are retained by the cache so that existing references to the them
  // remain valid. Note that this cannot be represented as a set/flat_set
  // because it can contain multiple modules that were loaded (then subsequently
  // unloaded) at the same base address.
  std::vector<std::unique_ptr<const Module>> inactive_non_native_modules_;

  // Auxiliary module provider, for lazily creating native modules.
  raw_ptr<AuxiliaryModuleProvider> auxiliary_module_provider_ = nullptr;
};

}  // namespace base

#endif  // BASE_PROFILER_MODULE_CACHE_H_
