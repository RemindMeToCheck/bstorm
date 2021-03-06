-- NOTE : 全ての引数はr_cpされていない前提
-- 引数からデータを持ち出してはいけない

local function r_copyarray(a)
  local r = {};
  for i=1,#a do r[i] = r_cp(a[i]); end
  return r;
end

local function r_strtodnhstr(s)
  local t = {};
  for i = 1, #s do
    t[i] = string.sub(s, i, i);
  end
  return t;
end

function r_type_eq(x, y)
  local tx = type(x);
  local ty = type(y);
  if tx == 'table' and ty == 'table' then
    if #x == 0 or #y == 0 then
      return true;
    else
      return r_type_eq(x[1], y[1]);
    end
  else
    return tx == ty;
  end
end

-- throws
function r_checknil(v, name)
  if v == nil then
    c_raiseerror("attempt to use empty variable `" .. name .. "'.");
  else
    return v;
  end
end

local r_ator = dnh_ator;

function r_tonum(x)
  local t = type(x);
  if t == 'number' then return x end
  if t == 'boolean' then return (x and 1 or 0) end
  if t == 'table' then return r_ator(x) end
  if t == 'string' then return c_chartonum(x) end
  if t == 'nil' then return 0 end
  return -1;
end

local function r_toint(x)
  local a = math.modf(r_tonum(x));
  return a;
end

function r_tobool(x)
  local t = type(x);
  if(t == 'boolean') then return x end
  if(t == 'number') then return x ~= 0 end
  if(t == 'table') then return #x ~= 0 end
  if(t == 'string') then return c_chartonum(x) ~= 0 end
  if(t == 'nil') then return false end
  return false
end

-- throws
function r_add(x, y)
  if type(x) == "table" then
    if type(y) ~= "table" then
      c_raiseerror("can't apply '+' to diffrent type values.");
    end
    if #x ~= #y then
      c_raiseerror("can't apply '+' to diffrent length arrays.");
    end
    local z = {};
    for i = 1, #x do
      z[i] = r_add(x[i], y[i]);
    end
    return z;
  else
    return r_tonum(x) + r_tonum(y);
  end
end

-- throws
function r_subtract(x, y)
  if type(x) == "table" then
    if type(y) ~= "table" then
      c_raiseerror("can't compare diffrent type values.");
    end
    if #x ~= #y then
      c_raiseerror("can't apply '-' to diffrent length arrays.");
    end
    local z = {};
    for i = 1, #x do
      z[i] = r_subtract(x[i], y[i]);
    end
    return z;
  else
    return r_tonum(x) - r_tonum(y);
  end
end

function r_multiply(x,y)
  return r_tonum(x) * r_tonum(y);
end

function r_divide(x,y)
  return r_tonum(x) / r_tonum(y);
end

function r_remainder(x,y)
  return r_tonum(x) % r_tonum(y);
end

function r_power(x,y)
  return r_tonum(x) ^ r_tonum(y);
end

function r_not(x)
  return not r_tobool(x);
end

function r_negative(x)
  return -r_tonum(x);
end

-- throws
function r_compare(x, y)
  if type(x) ~= type(y) then
    c_raiseerror("can't compare diffrent type values.");
  end

  if type(x) == "number" then
    if x < y then
      return -1;
    elseif x > y then
      return 1;
    else
      return 0;
    end
  end

  if type(x) == "string" then
    return r_compare(c_chartonum(x), c_chartonum(y));
  end

  if type(x) == "boolean" then
    return r_compare(r_tonum(x), r_tonum(y));
  end

  if type(x) == "table" then
    for i = 1, math.min(#x, #y) do
      local r = r_compare(x[i], y[i]);
      if r ~= 0 then return r end
    end
    return r_compare(#x, #y);
  end

  return 0;
end

function r_and(x, f)
  if r_tobool(x) then return f() end
  return x;
end

function r_or(x, f)
  if r_tobool(x) then return x end
  return f();
end

function r_cp(x)
  if type(x) == "table" then
    return r_copyarray(x)
  end
  return x;
end

function r_mconcatenate(a, b)
  if type(a) ~= "table" or type(b) ~= "table" then
    c_raiseerror("can't concat non-array values.");
  end

  if not r_type_eq(a, b) then
    c_raiseerror("can't concat diffrent type values.");
  end

  local j = #a + 1;
  for i = 1, #b do a[j] = r_cp(b[i]); j = j + 1; end
  return a;
end

-- throws
function r_concatenate(a, b)
  return r_mconcatenate(r_cp(a), b);
end

-- throws
function r_slice(a, s, e)
  if type(a) ~= "table" then
    c_raiseerror("can't slice non-array values.");
  end

  s = r_toint(s); e = r_toint(e);

  if s > e  or s > #a or e > #a or s < 0 or e < 0 then
    c_raiseerror("array index out of bounds.");
  end

  s = s + 1; e = e + 1;

  local r = {};
  local i = 1;
  for j = s, e-1 do
    r[i] = r_cp(a[j]);
    i = i + 1;
  end
  return r;
end

-- throws
function r_read(a, idx)
  if type(a) ~= "table" then
    c_raiseerror("attempt to index a non-array value.");
  end

  idx = r_toint(idx);

  if idx >= #a or idx < 0 then
    c_raiseerror("array index out of bounds.");
  end

  idx = idx + 1;

  return r_cp(a[idx]);
end

-- throws
function r_write1(a, idx, v)
  if type(a) ~= "table" then
    c_raiseerror("attempt to index a non-array value.");
  end

  idx = r_toint(idx);

  if idx >= #a or idx < 0 then
    c_raiseerror("array index out of bounds.");
  end

  idx = idx + 1;

  if not r_type_eq(a[1], v) then
    c_raiseerror("array element type mismatch.");
  end

  a[idx] = r_cp(v);
end

-- throws
function r_write(a, indices, v)
  for k=1, #indices do
    if type(a) ~= "table" then
      c_raiseerror("attempt to index a non-array value.");
    end

    local idx = r_toint(indices[k]);

    if idx >= #a or idx < 0 then
      c_raiseerror("array index out of bounds.");
    end

    idx = idx + 1;

    if k == #indices then
      if not r_type_eq(a[1], v) then
        c_raiseerror("array element type mismatch.");
      end

      a[idx] = r_cp(v);
    end

    a = a[idx];
  end
end

function r_successor(x)
  if type(x) == "number" then
    return x + 1;
  elseif type(x) == "string" then
    return c_succchar(x);
  elseif type(x) == "boolean" then
    return true;
  elseif type(x) == "table" then
    c_raiseerror("can't apply successor for array.");
  end
  return x;
end

function r_predecessor(x)
  if type(x) == "number" then
    return x - 1;
  elseif type(x) == "string" then
    return c_predchar(x);
  elseif type(x) == "boolean" then
    return false;
  elseif type(x) == "table" then
    c_raiseerror("can't apply predecessor for array.");
  end
  return x;
end

function r_absolute(x)
  return math.abs(r_tonum(x));
end

dnh_add = r_add;
dnh_subtract = r_subtract;
dnh_multiply = r_multiply;
dnh_divide = r_divide;
dnh_remainder = r_remainder;
dnh_power = r_power
dnh_not = r_not;
dnh_negative = r_negative;
dnh_compare = r_compare;
dnh_concatenate = r_concatenate;
dnh_index_ = r_read;
dnh_slice = r_slice;
dnh_successor = r_successor;
dnh_predecessor = r_predecessor;
dnh_absolute = r_absolute;

-- throws
function dnh_append(a,x)
  return r_concatenate(a, {x});
end

-- throws
function dnh_erase(a, i)
  if(type(a) ~= "table") then
      c_raiseerror("attempt to erase a non-array value.");
  end

  i = r_toint(i);

  if i >= #a or i < 0 then
    c_raiseerror("array index out of bounds.");
  end

  i = i + 1;

  local r = {};
  for j = 1, #a do
    if j ~= i then table.insert(r, r_cp(a[j])); end
  end
  return r;
end

function dnh_length(a)
  if type(a) ~= "table" then
    return 0;
  else
    return #a;
  end
end

--- math function ---

function dnh_min(x,y)
  return math.min(r_tonum(x), r_tonum(y));
end

function dnh_max(x,y)
  return math.max(r_tonum(x), r_tonum(y));
end

function dnh_log(x)
  return math.log(r_tonum(x));
end

function dnh_log10(x)
  return math.log10(r_tonum(x));
end

function dnh_cos(x)
  return math.cos(math.rad(r_tonum(x)));
end

function dnh_sin(x)
  return math.sin(math.rad(r_tonum(x)));
end

function dnh_tan(x)
  return math.tan(math.rad(r_tonum(x)));
end

function dnh_acos(x)
  return math.deg(math.acos(r_tonum(x)));
end

function dnh_asin(x)
  return math.deg(math.asin(r_tonum(x)));
end

function dnh_atan(x)
  return math.deg(math.atan(r_tonum(x)));
end

function dnh_atan2(y,x)
  return math.deg(math.atan2(r_tonum(y), r_tonum(x)));
end

math.randomseed(os.time()); -- FUTURE : Systemから与えられたシードで初期化

function dnh_rand(min, max)
  min = r_tonum(min); max = r_tonum(max);
  return math.random() * (max - min) + min;
end

function dnh_round(x)
  return math.floor(r_tonum(x) + 0.5);
end

dnh_truncate = r_toint;

dnh_trunc = r_toint;

function dnh_floor(x)
  return math.floor(r_tonum(x));
end

function dnh_ceil(x)
  return math.ceil(r_tonum(x));
end

function dnh_modc(x, y)
  return math.fmod(r_tonum(x), r_tonum(y));
end

function dnh_IntToString(n)
  n = r_toint(n);
  return r_strtodnhstr(string.format("%d", n));
end

dnh_itoa = dnh_IntToString

function dnh_rtoa(n)
  n = r_tonum(n);
  return r_strtodnhstr(string.format("%f", n));
end

script_event_type = -1;
script_event_args = {};

function dnh_GetEventType()
  return script_event_type;
end

function dnh_GetEventArgument(idx)
  -- 小数は切り捨てる
  idx = r_toint(idx) + 1;
  if idx < 1 or idx > #script_event_args then
    c_raiseerror("event arguments array index out of bounds.");
  end
  return r_cp(script_event_args[idx]);
end

--- task engine ---

local Task = {RUNNING = 0, SUSPENDED = 1}; Task.__index = Task;

function Task:create()
  local t = {func = nil, args = nil, state = Task.SUSPENDED};
  t.step = coroutine.wrap(function()
    while true do
      if t.args == nil then
        t.func();
      else
        t.func(unpack(t.args));
      end
      t.func = nil; t.args = nil;
      t.state = Task.SUSPENDED;
      coroutine.yield();
    end
  end);
  setmetatable(t, Task);
  return t;
end

function Task:resume()
  if self.state == Task.SUSPENDED then return; end
  self.step();
end

function Task:set_func(f, args)
  if self.state ~= Task.SUSPENDED then
    -- error!
  end

  self.func = f;
  self.args = args;
  self.state = Task.RUNNING;
end

local TaskManager = {}; TaskManager.__index = TaskManager;
function TaskManager:create(n)
  local tm = {running = {}, added = {}, pooled = {}};
  for i = 1, n do
    tm.pooled[i] = Task:create();
  end
  setmetatable(tm, TaskManager);
  return tm;
end

function TaskManager:register(f, args)
  local t = table.remove(self.pooled);
  if t == nil then
    t = Task:create();
  end
  t:set_func(f, args);
  t:resume();
  if t.state == Task.SUSPENDED then
    self.pooled[#self.pooled+1] = t;
  else
    self.added[#self.added+1] = t;
  end
end

function TaskManager:resumeAll()
  self.running, self.added = self.added, self.running;
  for i,t in ipairs(self.running) do
    t:resume();
    if t.state == Task.SUSPENDED then
      self.pooled[#self.pooled+1] = t;
    else
      self.added[#self.added+1] = t;
    end
    self.running[i] = nil;
  end
end

local task_manager = TaskManager:create(128);
local main_task = Task:create();

local function r_run(f)
  main_task:set_func(f, nil);
  while true do
    main_task:resume();
    if main_task.state == Task.SUSPENDED then
      break;
    end
    task_manager:resumeAll();
  end
end

function r_fork(func, args) task_manager:register(func, args) end

-- entry point --

function dnh_Loading() end
function dnh_Event() end
function dnh_Initialize() end
function dnh_MainLoop() end
function dnh_Finalize() end

function r_run_Loading() r_run(dnh_Loading); end
function r_run_Event() r_run(dnh_Event); end
function r_run_Initialize() r_run(dnh_Initialize); end
function r_run_MainLoop() r_run(dnh_MainLoop); end
function r_run_Finalize() r_run(dnh_Finalize); end