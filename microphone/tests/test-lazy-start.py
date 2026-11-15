"""Compile and execute the actual kext start() in a host model with no MMIO API."""
from pathlib import Path
import subprocess,tempfile
base=Path(__file__).resolve().parents[1]
s=(base/'kext/HP250Dmic/HP250Dmic.cpp').read_text()
a=s.index('bool HP250Dmic::start(IOService *provider) {')
z=s.index('\nvoid HP250Dmic::walkCapabilities()',a)
actual=s[a:z]
prefix=r'''
#include <cassert>
#include <cstdint>
static constexpr int kIOReturnSuccess=0;
struct IOService {virtual ~IOService(){};const char *getName(){return "FakeHP";}};
struct IOPCIDevice:IOService {int refs=0;void retain(){refs++;}};
struct FakeBase {bool start(IOService*){return true;}};
static bool failWorkLoop=false;
struct IOWorkLoop {static IOWorkLoop *workLoop(){static IOWorkLoop w;return failWorkLoop?nullptr:&w;}int addEventSource(void*){return 0;}};
struct IOCommandGate {static IOCommandGate *commandGate(void*){static IOCommandGate g;return &g;}};
#define super FakeBase
#define OSDynamicCast(T,p) dynamic_cast<T*>(p)
#define OSSafeReleaseNULL(p) p=nullptr
#define LOG(...) ((void)0)
static uint64_t mach_absolute_time(){return 100;}
class HP250Dmic:public FakeBase {
public:
    IOPCIDevice *pci=nullptr;
    bool readDsp=true,captureRequested=true,serviceMode=false,bootRequested=true,keepRunning=true;
    uint64_t loadedAt=0;
    IOWorkLoop *workLoop=nullptr;IOCommandGate *gate=nullptr;
    bool published=false,registered=false;
    void setProperty(const char*,const char*){}
    void publish(){published=true;assert(!bootRequested&&!captureRequested&&!readDsp);}
    void registerService(){registered=true;}
    bool start(IOService*);
};
'''
suffix=r'''
int main(){
    IOPCIDevice pci;HP250Dmic good;
    assert(good.start(&pci)&&good.published&&good.registered&&good.serviceMode);
    assert(good.loadedAt==100&&!good.keepRunning&&pci.refs==1);
    IOService wrong;HP250Dmic bad;assert(!bad.start(&wrong)&&!bad.registered);
    failWorkLoop=true;HP250Dmic noLoop;assert(!noLoop.start(&pci)&&!noLoop.registered);
}
'''
with tempfile.TemporaryDirectory(prefix='hp250-lazy-test-') as work:
    src=Path(work)/'test.cpp';exe=Path(work)/'test'
    src.write_text(prefix+actual+suffix)
    subprocess.run(['xcrun','clang++','-std=c++17','-fsanitize=address,undefined',str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Actual start(): service registration succeeds without a register/DSP/power-tree API; failure paths checked.')
