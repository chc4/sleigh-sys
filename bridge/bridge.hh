#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>
#include <iostream>

#include "../decompiler/address.hh"
#include "../decompiler/globalcontext.hh"
#include "../decompiler/loadimage.hh"
#include "../decompiler/opbehavior.hh"
#include "../decompiler/sleigh.hh"
#include "../decompiler/space.hh"
#include "../decompiler/translate.hh"

using std::make_unique;
using std::move;
using std::unique_ptr;

class RustPCodeEmit;

class RustPCodeEmitProxy : public PcodeEmit {
private:
  RustPCodeEmit *inner;

public:
  RustPCodeEmitProxy(RustPCodeEmit *emit) : inner(emit) {}

  virtual void dump(const Address &addr, OpCode opc, VarnodeData *outvar,
                    VarnodeData *vars, int4 isize);
};

class RustAssemblyEmit;
class RustAssemblyEmitProxy : public AssemblyEmit {
private:
  RustAssemblyEmit *inner;

public:
  RustAssemblyEmitProxy(RustAssemblyEmit *inner) : inner(inner) {}

  virtual void dump(const Address &addr, const string &mnem,
                    const string &body);
};

class RustLoadImage;

class RustLoadImageProxy : public LoadImage {
private:
  RustLoadImage *inner;

public:
  RustLoadImageProxy(RustLoadImage *inner)
      : LoadImage("nofile"), inner(inner) {}

  virtual void loadFill(uint1 *ptr, int4 size, const Address &address);
  virtual string getArchType(void) const { return "plain"; }
  virtual void adjustVma(long adjust);
};

class Decompiler : Sleigh {
private:
  unique_ptr<LoadImage> loadImage;
  unique_ptr<DocumentStorage> spec;
  ContextInternal context;

public:
  Decompiler(unique_ptr<LoadImage> loadImage, unique_ptr<DocumentStorage> spec)
      : Sleigh(loadImage.get(), &this->context), loadImage(move(loadImage)),
        spec(move(spec)) {
    this->initialize(*this->spec);

    // Initialize the stack address space
    AddrSpace *ram = getAddressSpaceByName("ram");
    const VarnodeData &rsp = getRegister("RSP");
    //this->addSpacebase(ram, "stack", rsp, sizeof(size_t), false, true);
    int4 ind = numSpaces();
    SpacebaseSpace *spc = new SpacebaseSpace(this,this,"stack",ind,sizeof(size_t),ram,rsp.space->getDelay()+1);
    insertSpace(spc);
    addSpacebasePointer(spc,rsp,sizeof(size_t),true);

  }

  int32_t translate(RustPCodeEmit *emit, uint64_t addr) const;
  int32_t disassemble(RustAssemblyEmit *emit, uint64_t addr) const;
  ContextDatabase *getContext() { return &this->context; }
  AddrSpace *getAddressSpace(int4 i) const { return this->getSpace(i); }
  AddrSpace *getAddressSpaceByName(const string &space) const {
      return this->getSpaceByName(space);
  }
  const VarnodeData &getRegisterByName(const string &reg) const {
      return ((const Translate*)this)->getRegister(reg);
  }
};

unique_ptr<Decompiler> newDecompiler(RustLoadImage *loadImage,
                                     unique_ptr<DocumentStorage> spec);
unique_ptr<Address> newAddress();
unique_ptr<ContextDatabase> newContext();
unique_ptr<DocumentStorage> newDocumentStorage(const std::string &s);

unique_ptr<string> owningGetRegisterName(const Translate* trans, AddrSpace *base,uintb off,int4 size);

uint32_t getAddrSpaceType(const AddrSpace &space);

uint32_t getVarnodeSize(const VarnodeData &data);
unique_ptr<Address> getVarnodeDataAddress(const VarnodeData &data);
