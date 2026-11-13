#include "RegionImport.h"
#include <cmath>

namespace choreo
{
namespace
{
struct Item { bool region = false; juce::String name; double start = 0, end = 0; };

juce::StringArray splitCsvLine(const juce::String& line, juce::juce_wchar delim)
{
    juce::StringArray out;
    juce::String cur;
    bool q = false;
    for (int i = 0; i < line.length(); ++i)
    {
        const auto c = line[i];
        if (q)
        {
            if (c == '"') { if (line[i + 1] == '"') { cur << '"'; ++i; } else q = false; }
            else cur << c;
        }
        else if (c == '"') q = true;
        else if (c == delim) { out.add(cur.trim()); cur.clear(); }
        else cur << c;
    }
    out.add(cur.trim());
    return out;
}

bool parseTime(juce::String s, double& out)
{
    s = s.trim();
    if (s.isEmpty()) return false;
    if (s.containsOnly("0123456789.-+")) { out = s.getDoubleValue(); return std::isfinite(out); }
    if (s.containsOnly("0123456789.:"))
    {
        auto parts = juce::StringArray::fromTokens(s, ":", "");
        if (parts.size() < 2 || parts.size() > 3) return false;
        double t = 0;
        for (auto& p : parts) t = t * 60.0 + p.getDoubleValue();
        out = t;
        return true;
    }
    return false;
}

int col(const juce::StringArray& header, std::initializer_list<const char*> names)
{
    for (auto* n : names)
        for (int i = 0; i < header.size(); ++i)
            if (header[i].trim().equalsIgnoreCase(n)) return i;
    return -1;
}
} // namespace

juce::String formatTimecode(double t)
{
    t = std::max(0.0, t);
    const int m = (int) (t / 60.0);
    return juce::String(m) + ":" + juce::String(t - m * 60.0, 3).paddedLeft('0', 6);
}

RegionImport sectionsFromRegionCsv(const juce::String& csvIn)
{
    RegionImport res;
    auto csv = csvIn;
    if (csv.startsWithChar((juce::juce_wchar) 0xFEFF)) csv = csv.substring(1);
    auto lines = juce::StringArray::fromLines(csv);
    lines.removeEmptyStrings(true);
    if (lines.size() < 2) { res.error = "The file contains no regions or markers."; return res; }

    juce::juce_wchar delim = ',';
    const auto& h0 = lines[0];
    if (h0.contains("\t")) delim = '\t';
    else if (h0.contains(";") && ! h0.contains(",")) delim = ';';

    const auto header = splitCsvLine(lines[0], delim);
    const int cType = col(header, { "type" });
    const int cId = col(header, { "#", "id", "index" });
    const int cName = col(header, { "name" });
    const int cStart = col(header, { "start", "position" });
    const int cEnd = col(header, { "end" });
    const int cLen = col(header, { "length", "len" });
    if (cStart < 0) { res.error = "No 'Start' column found. Use the Lua script to export, or an export from the Region/Marker Manager."; return res; }

    std::vector<Item> items;
    int badTime = 0;
    for (int li = 1; li < lines.size(); ++li)
    {
        const auto f = splitCsvLine(lines[li], delim);
        auto get = [&f] (int c) { return c >= 0 && c < f.size() ? f[c] : juce::String(); };
        Item it;
        it.name = get(cName);
        if (! parseTime(get(cStart), it.start)) { ++badTime; continue; }
        double e = 0, len = 0;
        const bool hasEnd = parseTime(get(cEnd), e);
        const bool hasLen = parseTime(get(cLen), len);
        if (hasEnd) it.end = e; else if (hasLen) it.end = it.start + len; else it.end = it.start;
        const auto type = get(cType).toLowerCase();
        const auto id = get(cId).toUpperCase();
        if (type.startsWith("reg") || type.startsWith("rgn")) it.region = true;
        else if (type.startsWith("mark") || type.startsWith("mrk")) it.region = false;
        else if (id.startsWithChar('R')) it.region = true;
        else if (id.startsWithChar('M')) it.region = false;
        else it.region = it.end > it.start + 1e-6;
        if (it.region && it.end <= it.start + 1e-6) it.region = false;
        items.push_back(it);
    }
    if (badTime > 0)
        res.notes.add(juce::String(badTime) + " line(s) skipped: time not readable. Set the REAPER ruler to seconds or h:m:s:ms.");

    std::vector<Item> regions, markers;
    for (auto& it : items) (it.region ? regions : markers).push_back(it);
    auto byStart = [] (const Item& a, const Item& b) { return a.start < b.start; };
    std::sort(regions.begin(), regions.end(), byStart);
    std::sort(markers.begin(), markers.end(), byStart);

    auto nameOr = [] (const juce::String& n, size_t i, const char* base) { return n.isNotEmpty() ? n.substring(0, 32) : juce::String(base) + " " + juce::String((int) i + 1); };

    if (! regions.empty())
    {
        // accept non-overlapping regions in order
        std::vector<Item> ok;
        for (auto& r : regions)
        {
            if (r.end - r.start < 0.1) { res.notes.add("Region '" + r.name + "' skipped: shorter than 0.1 s."); continue; }
            if (! ok.empty() && r.start < ok.back().end - 0.001)
            {
                res.notes.add("Region '" + r.name + "' skipped: overlaps '" + ok.back().name + "'.");
                continue;
            }
            ok.push_back(r);
        }
        int gaps = 0;
        for (size_t i = 0; i < ok.size(); ++i)
        {
            const bool last = i + 1 == ok.size();
            res.sections.push_back({ nameOr(ok[i].name, i, "Section"), ok[i].end - ok[i].start });
            if (! last && ok[i + 1].start - ok[i].end > 0.05)
            {
                res.sections.push_back({ "(gap)", ok[i + 1].start - ok[i].end });
                ++gaps;
            }
        }
        if (gaps > 0) res.notes.add(juce::String(gaps) + " gap(s) between regions became '(gap)' sections so the boundaries stay in sync. Delete or rename them if you like.");
        if (! ok.empty()) res.startOffset = ok.front().start;
        if (! markers.empty()) res.notes.add("Markers ignored: regions define the sections.");
    }
    else if (markers.size() >= 2)
    {
        for (size_t i = 0; i + 1 < markers.size(); ++i)
        {
            const double d = markers[i + 1].start - markers[i].start;
            if (d < 0.1) { res.notes.add("Marker '" + markers[i].name + "' skipped: next marker is closer than 0.1 s."); continue; }
            res.sections.push_back({ nameOr(markers[i].name, i, "Section"), d });
        }
        res.startOffset = markers.front().start;
        res.notes.add("Markers only: each section runs from one marker to the next; the last marker ends the form ('" + markers.back().name + "').");
    }
    if (res.sections.empty())
        res.error = "No usable regions found (or fewer than two markers).";
    return res;
}
} // namespace choreo
