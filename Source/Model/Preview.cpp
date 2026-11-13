#include "Preview.h"
#include "AnimatorMath.h"

namespace choreo
{
using Quat = juce::Quaternion<double>;

struct TimelinePreview::PreparedMove
{
    MClip src;
    MovementClip clip;
    AnimatorMath::MovementStartState st;
    V3 ref, end;
};

struct TimelinePreview::PreparedAct
{
    AClip src;
    ActionClip clip;
    Quat capQ;
    bool capHas;
    double capS;
    bool capSHas;
    Quat endQ;
    double endS;
};

static Quat identityQ() { return Quat(juce::Vector3D<double>(0, 0, 0), 1.0); }

TimelinePreview::TimelinePreview(const Timeline& tl, std::optional<V3> initialPos)
{
    std::vector<const MClip*> mv;
    for (auto& c : tl.movement) if (! c.muted) mv.push_back(&c);
    std::stable_sort(mv.begin(), mv.end(), [] (auto* a, auto* b) { return a->start < b->start; });

    std::optional<V3> pos = initialPos;
    for (auto* c : mv)
    {
        const V3 ref = pos ? *pos : (c->useStartPoint ? c->sp : V3());
        auto clip = toIcst(*c);
        auto st = AnimatorMath::computeMovementStartState(clip, ref);
        auto end = AnimatorMath::calculatePosition(clip, AnimatorMath::applyEasing(clip, 1.0), st, ref);
        moves.push_back({ *c, clip, st, ref, end });
        pos = end;
    }
    if (! moves.empty())
        firstPos = moves.front().src.useStartPoint ? moves.front().src.sp : moves.front().ref;
    else
        firstPos = initialPos ? *initialPos : V3();

    std::vector<const AClip*> ac;
    for (auto& c : tl.actions) if (! c.muted) ac.push_back(&c);
    std::stable_sort(ac.begin(), ac.end(), [] (auto* a, auto* b) { return a->start < b->start; });

    Quat q = identityQ();
    bool hasQ = false, hasS = false;
    double stretch = 1;
    for (auto* c : ac)
    {
        auto clip = toIcst(*c);
        const Quat capQ = q;
        const bool capHas = hasQ, capSHas = hasS;
        const double capS = stretch;
        const double pEnd = AnimatorMath::applyEasing(clip, 1.0);
        auto rot = AnimatorMath::computeClipRotation(clip, AnimatorMath::rotationPhase(pEnd, clip.repetitions, clip.palindrome), capQ, capHas);
        if (rot.hasRotation) { q = rot.orientation; hasQ = true; }
        const double cp = AnimatorMath::computeCycleState(pEnd, clip.repetitions, clip.palindrome).cycleProgress;
        for (auto& a : clip.actions)
            if (a.getAction() == ActionType::Stretch && a.getTiming() != TimingType::None)
            {
                stretch = AnimatorMath::calculateStretch(a, cp, clip.length, capS, capSHas);
                hasS = true;
            }
        acts.push_back({ *c, clip, capQ, capHas, capS, capSHas, q, stretch });
    }
}

TimelinePreview::~TimelinePreview() = default;

Sample TimelinePreview::sample(double tMs) const
{
    Sample s;
    s.pos = firstPos;
    for (auto& m : moves)
    {
        if (tMs < (double) m.src.start) break;
        if (tMs <= (double) m.src.end())
        {
            const double p = m.src.length > 0 ? juce::jlimit(0.0, 1.0, (tMs - (double) m.src.start) / (double) m.src.length) : 1.0;
            s.pos = AnimatorMath::calculatePosition(m.clip, AnimatorMath::applyEasing(m.clip, p), m.st, m.ref);
            s.activeMove = &m.src;
            break;
        }
        s.pos = m.end;
    }
    for (auto& a : acts)
    {
        if (tMs < (double) a.src.start) break;
        if (tMs <= (double) a.src.end())
        {
            const auto& c = a.clip;
            const double p = AnimatorMath::applyEasing(c, c.length > 0 ? juce::jlimit(0.0, 1.0, (tMs - (double) c.start) / (double) c.length) : 1.0);
            auto rot = AnimatorMath::computeClipRotation(c, AnimatorMath::rotationPhase(p, c.repetitions, c.palindrome), a.capQ, a.capHas);
            s.q = rot.hasRotation ? rot.orientation : a.capQ;
            const double cp = AnimatorMath::computeCycleState(p, c.repetitions, c.palindrome).cycleProgress;
            s.stretch = a.capS;
            for (auto& d : c.actions)
            {
                if (d.getAction() == ActionType::Stretch && d.getTiming() != TimingType::None)
                    s.stretch = AnimatorMath::calculateStretch(d, cp, c.length, a.capS, a.capSHas);
                if (d.getAction() == ActionType::Jitter) { s.jitter = d.getValue(); s.jitterSpeed = d.getJitterSpeed(); }
            }
            s.activeAct = &a.src;
            break;
        }
        s.q = a.endQ;
        s.stretch = a.endS;
    }
    return s;
}

V3 rotate(const Quat& q, V3 v)
{
    // v' = v + w t + u x t, t = 2 (u x v)
    const V3 u = q.vector;
    const double w = q.scalar;
    const V3 t = (u ^ v) * 2.0;
    return v + t * w + (u ^ t);
}

std::array<V3, 4> satellites(const Sample& smp, double R, int gi, double tSec)
{
    const double s = 0.13 * R * smp.stretch;
    const V3 base[4] = { { s, 0, 0 }, { 0, s, 0 }, { -s, 0, 0 }, { 0, -s, 0 } };
    std::array<V3, 4> out;
    auto jit = [&] (int k, double tt)
    {
        const juce::uint32 seed = (juce::uint32) ((juce::int64) (gi + 1) * 7919 + (juce::int64) k * 104729);
        return PerlinNoise::noise1D(seed, tt);
    };
    for (int k = 0; k < 4; ++k)
    {
        V3 v = rotate(smp.q, base[k]);
        if (smp.jitter > 0)
        {
            const double tt = tSec * smp.jitterSpeed;
            v = v + V3 { smp.jitter * 2 * jit(k * 3, tt), smp.jitter * 2 * jit(k * 3 + 1, tt), smp.jitter * 2 * jit(k * 3 + 2, tt) };
        }
        out[(size_t) k] = smp.pos + v;
    }
    return out;
}
} // namespace choreo
