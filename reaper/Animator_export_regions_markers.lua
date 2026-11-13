-- Animator Choreographer: export REAPER regions and markers as CSV.
-- Run via Actions > Show action list > New action > Load ReaScript.
-- Writes Animator_<project>_regions_markers.csv next to the project file
-- (times in seconds). Import it in the app: step "Form" > "Import regions (CSV)".

local proj = 0
local _, num_markers, num_regions = reaper.CountProjectMarkers(proj)
if num_markers + num_regions == 0 then
  reaper.ShowMessageBox("The project has no regions or markers.", "Animator Choreographer", 0)
  return
end

local name = reaper.GetProjectName(proj):gsub("%.[Rr][Pp][Pp]$", "")
if name == "" then name = "Untitled" end
local dir = reaper.GetProjectPath(""):gsub("[/\\][^/\\]*$", "") -- project folder (parent of the media folder)
local sep = package.config:sub(1, 1)
local prefix = name:lower():find("^animator") and "" or "Animator_"
local path = dir .. sep .. prefix .. name .. "_regions_markers.csv"

local function num(x) return (string.format("%.6f", x):gsub(",", ".")) end
local function q(s) return '"' .. tostring(s):gsub('"', '""') .. '"' end

local rows = {}
local ri, mi = 0, 0
local i = 0
while true do
  local ret, isrgn, pos, rgnend, mname = reaper.EnumProjectMarkers3(proj, i)
  if ret == 0 then break end
  if isrgn then
    ri = ri + 1
    rows[#rows + 1] = { pos, string.format("region,%d,%s,%s,%s,%s", ri, q(mname), num(pos), num(rgnend), num(rgnend - pos)) }
  else
    mi = mi + 1
    rows[#rows + 1] = { pos, string.format("marker,%d,%s,%s,,", mi, q(mname), num(pos)) }
  end
  i = i + 1
end
table.sort(rows, function(a, b) return a[1] < b[1] end)

local f, err = io.open(path, "w")
if not f then
  reaper.ShowMessageBox("Could not write:\n" .. path .. "\n" .. tostring(err), "Animator Choreographer", 0)
  return
end
f:write("type,index,name,start,end,length\n")
for _, r in ipairs(rows) do f:write(r[2], "\n") end
f:close()
reaper.ShowMessageBox(string.format("%d regions, %d markers written to:\n%s", ri, mi, path), "Animator Choreographer", 0)
