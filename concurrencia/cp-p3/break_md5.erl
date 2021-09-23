%Practica 3 de Concurrencia
% Fernando Seara Romera && Eduardo Pérez Fraguela

-module(break_md5).
-define(PASS_LEN, 6).
-define(UPDATE_BAR_GAP, 1000).
-define(BAR_SIZE, 40).
-define(NUM_PROCS, 3).

-export([break_md5s/1, break_md5/1, break_md5/5, break_md5/6, pass_to_num/1, num_to_pass/1]).
-export([progress_loop/2]).

% Base ^ Exp
pow_aux(_Base, Pow, 0) -> Pow;
pow_aux(Base, Pow, Exp) when Exp rem 2 == 0 ->
    pow_aux(Base*Base, Pow, Exp div 2);
pow_aux(Base, Pow, Exp) -> pow_aux(Base, Base * Pow, Exp - 1).

pow(Base, Exp) -> pow_aux(Base, 1, Exp).

%% Number to password and back conversion
num_to_pass_aux(_N, 0, Pass) -> Pass;
num_to_pass_aux(N, Digit, Pass) ->
    num_to_pass_aux(N div 26, Digit - 1, [$a + N rem 26 | Pass]).

num_to_pass(N) -> num_to_pass_aux(N, ?PASS_LEN, []).

pass_to_num_aux([], Num) -> Num;
pass_to_num_aux([C|T], Num) -> pass_to_num_aux(T, Num*26 + C-$a).

pass_to_num(Pass) -> pass_to_num_aux(Pass, 0).

%% Hex string to Number
hex_char_to_int(N) ->
    if (N >= $0) and (N =< $9) -> N-$0;
       (N >= $a) and (N =< $f) -> N-$a+10;
       (N >= $A) and (N =< $F) -> N-$A+10;
       true -> throw({not_hex, [N]})
    end.

hex_string_to_num_aux([], Num) -> Num;
hex_string_to_num_aux([Hex|T], Num) ->
    hex_string_to_num_aux(T, Num*16 + hex_char_to_int(Hex)).

hex_string_to_num(Hex) -> hex_string_to_num_aux(Hex, 0).

%% Progress bar runs in its own process
progress_loop(N, Bound) ->
    receive
        stop -> 
        io:format("Finalizando proceso Progress Bar ~w ~n",[self()]),
        ok;
        {progress_report, Checked} ->
            N2 = N + Checked,
            Full_N = N2 * ?BAR_SIZE div Bound,
            Full = lists:duplicate(Full_N, $=),
            Empty = lists:duplicate(?BAR_SIZE - Full_N, $-),
            io:format("\r[~s~s] ~.2f%", [Full, Empty, N2/Bound*100]),
            progress_loop(N2, Bound)
    end.

%% Arranca los procesos
start_process(Pids, _, NumProcs, NumProcs, _) ->
	Pids;
	
start_process(Pids, Num_Hashes, NumProcs, N, Progress_Pid) ->
    Bound = pow(26, ?PASS_LEN),
    Bloque = Bound div ?NUM_PROCS,
	Inferior = Bloque * N,
	Superior = if
		N==NumProcs-1 -> Bound;
		true -> Bloque * (N+1)
	end,
	Pid = spawn(?MODULE,break_md5,[Num_Hashes, Inferior, Superior, Progress_Pid, self(), inicio]),
	start_process(Pids ++ [Pid],Num_Hashes, NumProcs, N+1, Progress_Pid).

%% break_md5/2 iterates checking the possible passwords
break_md5(Hashes, N, Bound, Progress_Pid, Principal, _) ->
	io:format("Proceso ~w buscando posibles contraseñas ~n",[self()]),
	break_md5(Hashes, N, Bound, Progress_Pid, Principal).

break_md5([], _, _, _, _) -> 
	io:format("Contraseñas encontradas. Finalizando proceso ~w ~n",[self()]),
	ok; % No more hashes to find
	
break_md5(Hashes,  N, N, _, _) ->
io:format("Checked every possible password. Finalizando proceso ~w ~n",[self()]),
	{not_found, Hashes}; % Checked every possible password

break_md5(Hashes, N, Bound, Progress_Pid, Principal) ->
    if N rem ?UPDATE_BAR_GAP == 0 ->
            Progress_Pid ! {progress_report, ?UPDATE_BAR_GAP};
       true ->
            ok
    end,
    receive
		{borrar, HashABorrar} ->
			L = lists:delete(HashABorrar,Hashes)
		after 0 ->
			L = Hashes
    end,
    Pass = num_to_pass(N),
    Hash = crypto:hash(md5, Pass),
    Num_Hash = binary:decode_unsigned(Hash),
    case lists:member(Num_Hash, L) of
        true ->
            io:format("\e[2K\r~.16B: ~s~n", [Num_Hash, Pass]),
            Principal ! {encontrada, Num_Hash, self()},
            break_md5(lists:delete(Num_Hash, L), N+1, Bound, Progress_Pid, Principal);
            
        false ->
            break_md5(L, N+1, Bound, Progress_Pid, Principal)
    end.

%% Break a list of hashes
break_md5s(Hashes) ->
    Bound = pow(26, ?PASS_LEN),
    Progress_Pid = spawn(?MODULE, progress_loop, [0, Bound]),
    Num_Hashes = lists:map(fun hex_string_to_num/1, Hashes),
    Breakers_Pids = start_process([],Num_Hashes, ?NUM_PROCS, 0, Progress_Pid),
	Res = notify(Breakers_Pids,Num_Hashes),
    Progress_Pid ! stop,
    Res.

%% Break a single hash
break_md5(Hash) -> break_md5s([Hash]).

%% Avisa cuando encuentra una contraseña
notify(_,[]) ->
	ok;
notify(Process,Num_Hashes) ->
	receive
		{encontrada, Hash, PorPid} ->
			[B ! {borrar, Hash} || B <- Process, B/=PorPid],
			notify(Process,lists:delete(Hash,Num_Hashes))
	end.
