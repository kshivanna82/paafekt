#include "MeshGenerator.h"
bool BuildMeshFromDepthMap(const std::vector<float>& D, int W, int H,
                           TArray<FVector>& V, TArray<int32>& T, float DepthScale, int Step)
{
    if (W<=1 || H<=1 || (int)D.size()<W*H) return false;
    V.Reset(); T.Reset();
    const int GW=(W-1)/Step+1, GH=(H-1)/Step+1;
    V.Reserve(GW*GH);
    for(int y=0;y<H;y+=Step) for(int x=0;x<W;x+=Step){
        float z=D[y*W+x]*DepthScale; V.Add(FVector(float(x-W*0.5f), float(y-H*0.5f), z));
    }
    auto Idx=[GW](int cx,int cy){return cy*GW+cx;};
    for(int cy=0;cy<GH-1;++cy) for(int cx=0;cx<GW-1;++cx){
        int i0=Idx(cx,cy), i1=Idx(cx+1,cy), i2=Idx(cx,cy+1), i3=Idx(cx+1,cy+1);
        T.Add(i0); T.Add(i2); T.Add(i1); T.Add(i1); T.Add(i2); T.Add(i3);
    }
    return true;
}
