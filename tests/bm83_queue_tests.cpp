#include "nightwave/bm83_command_queue.h"
#include <cassert>
#include <cstdio>
using namespace nightwave;
int main() {
    Bm83CommandQueue q; Bm83Packet p; p.size=2; p.body[0]=0x44; p.body[1]=4;
    assert(!q.enqueue(p,1));
    for (unsigned i=0;i<4;++i) assert(q.enqueue(p,0));
    assert(!q.enqueue(p,0));
    for (unsigned i=0;i<4;++i) {
        auto v=q.pending(10); assert(v.size==6 && v.data[0]==0xaa);
        assert(!q.acknowledge(0x44,true,0,10));
        assert(q.sent(1,11)); v=q.pending(12); assert(v.size==5 && v.data[0]==0);
        assert(q.sent(5,13)); assert(q.pending(14).size==0);
        assert(!q.acknowledge(0x43,true,0,14));
        assert(!q.acknowledge(0x44,true,1,14));
        assert(q.acknowledge(0x44,true,0,14));
    }
    assert(q.size()==0 && q.pending(20).size==0);
    assert(q.enqueue(p,0)); q.pending(100); q.tick(1100);
    assert(q.state()==Bm83CommandQueue::State::kFault && !q.enqueue(p,0));
    q.reset_after_resync(); assert(!q.enqueue(p,0)); assert(q.enqueue(p,1));
    q.pending(0xfffffff0u); assert(q.sent(6,0xfffffff0u));
    assert(q.acknowledge(0x44,true,1,20));
    assert(q.enqueue(p,1)); q.pending(100); q.sent(6,100);
    assert(!q.acknowledge(0x44,true,1,1100));
    q.reset_after_resync(); assert(q.enqueue(p,2)); q.pending(0);
    assert(!q.sent(7,0) && q.state()==Bm83CommandQueue::State::kFault);
    q.reset_after_resync(); p.size=257; assert(!q.enqueue(p,3));
    p.size=0; assert(!q.enqueue(p,3)); p.size=256;
    assert(q.enqueue(p,3)); assert(q.pending(0).size==260); assert(q.sent(260,0));
    assert(!q.acknowledge(0x44,false,3,1)); assert(q.size()==0);
    std::puts("BM83 bounded queue, partial write, timeout and ACK tests passed");
}
