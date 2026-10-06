#include <cstdlib>
#include <iostream>
#include <vector>
#include "nightwave/hardware_config.h"
#include "nightwave/button_gestures.h"
#include "nightwave/st7789_display.h"
using namespace nightwave;
namespace {
int failures=0;
#define CHECK(x) do { if (!(x)) { ++failures; std::cerr<<__LINE__<<": "<<#x<<'\n'; } } while(false)
struct Bus : St7789Bus {
    struct Op { std::uint8_t code; std::vector<std::uint8_t> data; };
    std::vector<Op> ops;
    std::vector<std::uint32_t> waits;
    std::size_t pixel_bytes=0;
    int calls=0, fail_call=-1;
    bool lit=false, connected=false;
    bool ok() { return calls++ != fail_call; }
    bool connect() override { return connected=ok(); }
    void disconnect() override { connected=false; }
    bool reset(bool) override { return ok(); }
    void delay_ms(std::uint32_t ms) override { waits.push_back(ms); }
    bool command(std::uint8_t code,const std::uint8_t* bytes,std::size_t n) override {
        if (!ok()) return false;
        Op op{code,{}}; if (n) op.data.assign(bytes,bytes+n); ops.push_back(op); return true;
    }
    bool pixels(const std::uint8_t* bytes,std::size_t n) override {
        CHECK(bytes && n<=SelectedDisplayProfile::tile_bytes && n%2==0);
        if (!ok()) return false;
        pixel_bytes+=n; return true;
    }
    bool backlight(bool on) override { if (!ok()) return false; lit=on; return true; }
};
}
int main() {
    static_assert(hardware::kReferenceBoard);
    static_assert(hardware::pins_are_unique() && hardware::pins_avoid_restricted_set());
    static_assert(hardware::kHeadphoneEnable==39 && hardware::kDacMute==18);
    ButtonGestures keys;
    std::vector<ButtonEvent> events;
    const auto emit=[&](const ButtonEvent& e){events.push_back(e);};
    keys.sample(1,0,emit); keys.sample(1,1000,emit); CHECK(events.empty()); // held at boot
    keys.sample(0,1001,emit); keys.sample(1,1010,emit); keys.sample(1,1040,emit);
    keys.sample(0,1050,emit); keys.sample(0,1080,emit);
    CHECK(events.size()==1 && events[0].button==ButtonId::kPrevious);
    keys.sample(2,1100,emit); keys.sample(2,1130,emit); keys.sample(2,1830,emit);
    CHECK(events.size()==2 && events.back().gesture==ButtonGesture::kLongPress);
    keys.sample(0,1840,emit); keys.sample(0,1870,emit); CHECK(events.size()==2);
    keys.sample(4,2000,emit); keys.sample(4,2030,emit); keys.invalidate();
    keys.sample(4,4000,emit); keys.sample(0,4100,emit); keys.sample(0,4200,emit);
    CHECK(events.size()==2); // recovery must not synthesize release or long press
    keys.sample(4,4300,emit); keys.sample(4,4330,emit); keys.sample(0,4340,emit); keys.sample(0,4370,emit);
    CHECK(events.size()==3);
    Bus bus; St7789Display display(bus);
    CHECK(!display.present({})); CHECK(display.begin()); CHECK(bus.lit);
    CHECK(bus.pixel_bytes==240*280*2);
    bool first=false,last=false;
    for (const auto& op:bus.ops) if(op.code==0x2b) {
        if(op.data==std::vector<std::uint8_t>{0,20,0,35}) first=true;
        if(op.data==std::vector<std::uint8_t>{1,36,1,43}) last=true;
    }
    CHECK(first && last); CHECK(bus.waits==std::vector<std::uint32_t>({100,100,100,120}));
    CHECK(display.sleep(true) && !bus.lit); CHECK(!display.present({}));
    CHECK(display.sleep(false) && !bus.lit);
    DisplayFrame lyrics; lyrics.lyrics_view=true; lyrics.lyric_current="a long line of lyrics that should wrap safely onto multiple rows";
    lyrics.lyric_next="UTF-8: \xc3\xa9"; lyrics.title="Nightwave";
    CHECK(display.present(lyrics) && bus.lit);
    bus.fail_call=bus.calls+2;
    CHECK(!display.present(lyrics) && !bus.lit && !bus.connected);
    CHECK(display.begin());
    // Exercise every init/frame transfer failure point, with safe darkness and
    // no persistent connected state after a failed initialization.
    Bus count; St7789Display counted(count); CHECK(counted.begin());
    for(int i=0;i<count.calls;++i) {
        Bus broken; broken.fail_call=i; St7789Display d(broken);
        CHECK(!d.begin()); CHECK(!broken.connected && !broken.lit);
    }
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
