// Engine stress test + printouts.  g++ -std=c++17 -ISource tests/engine_test.cpp -o /tmp/et && /tmp/et
#include "engine/Engine.h"
#include "engine/Groove.h"
#include <cstdio>
using namespace kc;
static const char* nn(int p){ static char b[16]; static const char* N[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}; snprintf(b,16,"%s%d",N[p%12],p/12-2); return b; }
int main(){
  auto& L=Library::get(); long runs=0, bad=0, empty=0;
  Rng r(7);
  for(size_t pi=0; pi<L.progs.size(); ++pi) for(int k=0;k<6;k++){
    Settings s; s.prog=(int)pi; s.rhythm=r.below((int)L.rhythms.size()); s.key=r.below(12); s.voicing=r.below(10); s.octave=r.below(5)-2;
    s.complexity=r.below(101); s.variation=r.below(101); s.barsMode=(int[]){0,1,2,4,8,16}[r.below(6)]; s.rateMode=r.below(5); s.bass=r.below(7);
    s.dynamics=r.below(101); s.gate=10+r.below(141); s.seed=1+r.below(9999); s.shape=r.below(9);
    auto P=generate(s); runs++;
    if(P.events.empty()) empty++;
    for(auto&e:P.events){ if(e.pitch<0||e.pitch>127||e.vel<1||e.vel>127||e.step<0||e.step>=P.lengthSteps||e.len<=0||!std::isfinite(e.len)) { if(bad++<5) printf("BAD prog %zu ev %f %f %d %d\n",pi,e.step,e.len,e.pitch,e.vel);} }
    if(P.lengthSteps%16||P.lengthSteps<16||P.lengthSteps>256) { bad++; printf("len %d\n",P.lengthSteps);} 
    for(auto&c:P.chords) if(c.pitches.empty()) { bad++; printf("empty voicing %s\n", c.name.c_str()); }
  }
  printf("runs %ld bad %ld empty %ld\n", runs,bad,empty);
  // samples
  int picks[]={0,40,120,300,700,760,900,1050,1100,1200,1300,1380};
  for(int pi:picks){ if(pi>=(int)L.progs.size()) continue; Settings s; s.prog=pi; s.key=9; 
    int g=L.progs[pi].genre; for(size_t ri=0;ri<L.rhythms.size();ri++) if(L.rhythms[ri].genre==g && L.rhythms[ri].sustain==0){ s.rhythm=(int)ri+ (pi%7); break; }
    auto P=generate(s); printf("\n[%s] %s | %s | %d steps\n", P.styleName.c_str(), P.progName.c_str(), P.rhythmName.c_str(), P.lengthSteps);
    for(auto&c:P.chords){ printf("   %-12s @%5.1f len %4.1f :", c.name.c_str(), c.start, c.len); for(int p:c.pitches) printf(" %s",nn(p)); printf("\n"); }
  }
  // groove numbers
  GrooveSettings g; g.mode=SwingMode::TR909;
  for(int k=1;k<=7;k++){ g.shuffle909=k; double d=grooveDelay(g,1,false,124,0); printf("909 shuffle %d: delay %.4f beats = swing %.1f%%\n",k,d,(0.25+d)/0.5*100);} 
  g.mode=SwingMode::MPC; for(double p:{50.,54.,58.,62.,66.,71.,75.}){ g.swingPct=p; double d=grooveDelay(g,1,false,124,0); printf("MPC %g%%: delay %.4f beats (%.0f ticks)\n",p,d,d*96);} 
  g.mode=SwingMode::Human; g.swingPct=58; for(double bpm:{90.,124.,140.}){ double a=grooveDelay(g,1,false,bpm,1), b=grooveDelay(g,0,false,bpm,1); printf("Human 58%% @%g: off %.4f beats, downbeat %.4f\n",bpm,a,b);} 
}
