local mp = require 'mp'
local output = assert(io.open(assert(os.getenv('VSR_PERF_CSV')), 'w'))
local start = mp.get_time()
output:write('wall_s,position_s,dropped,decoder_dropped,hwdec,width,height,display_fps,estimated_fps\n')
local timer
timer = mp.add_periodic_timer(1, function()
    local elapsed = mp.get_time() - start
    output:write(string.format('%.6f,%.6f,%d,%d,%s,%d,%d,%.6f,%.6f\n',
        elapsed, mp.get_property_number('time-pos', -1),
        mp.get_property_number('frame-drop-count', 0),
        mp.get_property_number('decoder-frame-drop-count', 0),
        mp.get_property('hwdec-current', 'unknown'),
        mp.get_property_number('osd-width', 0), mp.get_property_number('osd-height', 0),
        mp.get_property_number('display-fps', 0), mp.get_property_number('estimated-vf-fps', 0)))
    output:flush()
    if elapsed >= tonumber(os.getenv('VSR_PERF_SECONDS') or '65') then
        timer:kill(); output:close(); mp.commandv('quit')
    end
end)
