# Current State

Reconciled: 2026-10-30 (UTC)

## Active task

Find what players feel as lag when the server is full, by measurement,
before changing more code. Live since 2026-10-29 14:39:24 UTC: `c9aa087`.

## Why this serves the goal

Lag-free play with stock clients. The measurements so far say the server
code keeps up; see "Lag findings" below.

## Status

This clone's `origin` is `pstngh/openmohaa-server`; `main` = `origin/main`.
Since the PR #1 merge (`5cc65d2`):

- `9407d96` PM_CmdScale divides by the raw usercmd values like AA 1.11,
  SH 2.15 and BT 2.40 (backpedal and strafe were 25%/18% too fast, and
  stock clients mispredicted). `test_pm_cmdscale` pins the stock speeds.
  OpenMoHAA clients keep the old formula until upstream openmohaa changes.
- `14c967a` `sv_frameStats` (default 60 s): per-minute main-thread work per
  tick while humans are connected. Adds `Sys_Microseconds`.
- `c9aa087` second line: snapshots sent, held by rate, held by fragments,
  worst player, snapshot size.
- Top speed rounded like AA 1.11 (`bg_speed.h`, always on): SMG 262 and
  rifles 257 at sv_dmspeedmult 1.1 (were 264 / 258). Live since the
  2026-10-30 04:00 UTC daily reboot (game.so built at `eb1d721`).
- `458da3a` snapshot entity ring sized for a full server
  (`SNAPSHOT_RING_ENTITIES` 256 per client per backup frame, 160 MB);
  the snapshot stats line ends with how many seconds the ring covers.
- `e41c1a0` missing TIKI models (players' custom skins) are searched
  for once per map instead of at every respawn.
- Live since 2026-10-30 10:14:19 UTC (`omohaaded` at `e41c1a0`, game.so
  at `eb1d721`), with `sv_minRate 50000` in `server_opm.cfg`. First
  minutes at 2-3 humans: ring covers 115-214 s.
- `029e45f` a player's missing custom model is skipped until the map
  changes (no "Bad model name" at every respawn). `1d358bd` loopback rcon
  (127.0.0.0/8, ::1) has its own bucket of 50 refilling at 50/s; others
  keep 10 refilling at 1/s.
- Live since 2026-10-30 11:01:10 UTC: `omohaaded` and `game.so` at
  `1d358bd`; `main/z_forteffa.pk3` without the "mam" mod leftovers (its
  death-box scripts and the Spearhead-only precache; the package is not
  in this repo; the previous one is in
  `.deploy-backups/prod-pre-1d358bd-20261030T105919Z/`). The guard env
  (`/etc/moharena/guard-aa.env`) has `MOHARENA_AC_AUDIT_INTERVAL=30` and
  `MOHARENA_GUARD_HEARTBEAT_INTERVAL=60`. First minutes: rcon drops and
  guard timeouts 0 (were ~36/min), no script errors at map loads.

The 2026-10-26 packet-demo commits were rolled back by JayRewd on
2026-10-28 (force-push of both repos to `5cc65d2`), on purpose. JayRewd's
fork has 4 newer profiler/bench commits (2026-10-29) that are not merged;
the owner said to ignore them for now.

### Live server and deploys

- VPS `forte-openmohaa` (Debian 13, glibc 2.41, 2 vCPU KVM). Game in
  `/home/debian/moh`, `mohaa.service`, port 12203, `sv_fps 20`,
  `sv_maxclients 32`, 5 roombas, up to 11 regular SMG bots below 16 players.
  `sv_minRate` in `main/server_opm.cfg`: 20000 from 2026-10-29, 50000
  from 2026-10-30 10:14 UTC.
- Builds from a newer distro do not run there (Ubuntu glibc 2.43). Build
  `omohaaded` on a Debian 13 host (`openmohaa-bot-vps` was used: send the
  commit with `git archive`, build in /tmp, delete afterwards). `game.so`
  only changes when game code does.
- Deploy: back up to `.deploy-backups/prod-pre-<commit>-<ts>/`, stage in
  `.deploy-stage/<commit>-<date>/`, run `.deploy-stage/smoke.sh` on a spare
  loopback port, then `mv` the binary into place and restart. A restart
  disconnects everyone; check the player count first.
- The VM reboots daily at 00:00 America/New_York (`mohaa-reboot.timer`).

### Lag findings (2026-10-29)

- The VM is paused by the host every day at 02:55-02:57 UTC for 3.5-21 s
  (kernel `clocksource: Long readout interval`, game hitch warnings), since
  at least 2026-10-12 and getting longer. Provider side, likely a scheduled
  backup or snapshot; not fixable in code.
- Evening of 2026-10-29, peak 26 humans: work per tick avg 6-7 ms at 20-26
  humans; 15 minutes had a tick over 25 ms, only two over 50 ms (62.9 ms at
  a map change, 55.5 ms frame time at 22:13 mid-game).
- Choke for players already in the game is about 1-2% at peak (rate
  ~0.8-1%, fragments ~0.6-0.8%). Snapshots over 4 KB and players held over
  half a minute are almost all joiners getting their first full snapshot.
- CGM buffer overflows (effects dropped for one player, ~1-2 a minute at
  peak) rise with player count but only loosely with choke; cause open.
- UDP receive drops happen only during map changes and the VM freeze.
- Client rates: 94% connect with 20000-30000.

### In the tree (kept)

- Navmesh disk cache `navmesh/<map>.bin` (homepath), keyed by BSP length +
  MD4 + `NAVMESH_CACHE_VERSION`; skipped with `developer`. Unit test
  `test_navmesh_cache`. Bump the version on any nav build change.
- Nav region labeling; `FindPathAway` skips unreachable islands; SMG-bot idle
  wander uses `FindPathNear` within its region (roaming looks the same).
- Per-Listener pending-event count short-circuits cancel/pending scans.
- State machines: resolved next-state pointers and pretokenized commands.
- FFA: score string cached per frame; top score computed once per frame.
- AI events skipped when no Actor exists; with `sv_netoptimize` off,
  `FootstepMain` only makes stock's equipment `random()` roll (step detection
  and the foot trace still run, so the `rand()` stream matches stock).
- `CanFall` trace moved from every frame into its conditional.
- Stack `EV_Touch` events; `CalcBlend` skips `pointcontents` when unused.
- Regular bots added/removed one per 250 ms.
- Engine: monotonic `Sys_Milliseconds`; load time no longer replayed as
  catch-up frames (`Com_ResetFrameTime`, map_restart guard); one log write
  per line; idle clients skip `SV_RateMsec`; sound index looked up once;
  bucket round-robin and reclaim by type; CGM datatypes buffer sized right
  (was a heap overflow); GameSpy queries capped per frame and rate-limited
  per address plus their own global bucket; dropped-gamestate resends at
  most once per second (never dropped); non-PVS sound pass skipped when off.
- Dead code removed (`CreateBeam`, `Body::Damage`, blood spurt helpers).

### Reverted in this review (player-visible)

- Explosion `SVF_NOCLIENT`: stock `fx_tank/stuka/bike_explosion.tik` are
  `dontdraw` yet draw client-side emitters (checked in Pak0).
- CGM discards for loading/downloading clients: stock delivers them on
  entry (e.g. script HUD draws).
- Gamestate "drop after 8 resends": lossy players could fail to join.
- `SpawnBloodyGibs` now always played `snd_decap`; restored stock body.
- `TouchStuff`: stock skips touch index > entity number; kept as O(1) check.
- SMG bots: combat retry back to 100 ms; jump/edge probes every frame again.

### Fight-load pass (2026-10-25)

Profiled a bot fight first, then kept only exact equivalences (D-013):

- `SV_FindIndex` remembers name -> index per configstring range, invalidated
  by a per-range generation that every write bumps. Barrels leaking in a
  fight called `LoopSound` (a full sound-name scan) every think.
- `MSG_WriteDeltaEntity`: byte-identical states take the "no change" path
  without the per-field pass. Field compares are inline for 1/2/4 bytes.
- Huffman: `msgHuff.compressor` is fixed after `MSG_initHuffman`, so
  `Huff_offsetTransmitCode` sends each byte from a code table (same bits,
  same stop at maxoffset). Unit test `test_huffman_table`.
- `MSG_SetBroadcastVisible` reuses each client's leaf area/cluster while its
  origin and `sv.serverId` are unchanged.
- `G_RunFrame` skips the Actor pre-animate walk when no Actor exists.
- Rejected: bounding the `SV_Trace` entity box by the world hit. The side-by-
  side check found a bullet hit it changed (bone spheres reach outside the
  entity box), and the rest depends on stale link boxes. Kept stock.
- Bench harness (scratch only, not in the tree): SMG bots without humans,
  roombas as stand-in clients with simulated delta snapshots and CGMs,
  gperftools profiler. Effect: about 20% less server time per frame.

### Fight-load pass 2 (2026-10-25)

Owner's target: slowest frame 0.8–1.2 ms on the 40-bot bench, Release build,
exact changes only (no PGO). Per-frame phase timers showed snapshot sending
(~0.43 ms in every frame) and bot thinking (spiking to ~1 ms) as the drivers.

- Bit writes go up to a byte at a time (`Huff_putBits`, chunked
  `Huff_offsetTransmitCode`); same bits and same `maxoffset` stop.
  `test_huffman_table` also compares `Huff_putBits` with `Huff_putBit`.
- `SV_WriteDeltaEntityShared` (`sv_snapshot.c`): one encoding per
  (from, to, force, frame time, protocol) per frame, bit-copied to each client
  (~94% reuse). Near-full messages use the normal call, so overflow is stock.
- `SV_SortSnapshotEntities`: skips `qsort` when the list is ascending or only
  the client's own entity is out of place; duplicates still reach `qsort`.
- `NavigationObstacleMap::IsValidEntity` tests cheap flags first and calls
  `IsSpecialEntity` only when the answer depends on it.
- `SV_SoundIndex` appends the `streamed` digit without `va`.
- Bench result (3 × 120 s, Release): average 1.05 → 0.84 ms, p99 1.67 →
  1.46 ms, worst ~2.3 → ~1.9 ms. The rest of the worst frames is roomba AI
  (20 roombas in the bench vs 5 live): staggered sight/use traces plus
  `Pmove`. Changing that cadence would change bot behavior, so it is left.
- Live-like bench (owner's chosen target: 5 thinking roombas, 15 idle
  stand-in clients, 20 SMG bots; `BENCH_THINKING_ROOMBAS=5`): average
  ~0.63 ms, p99 ~1.1 ms, worst per 10 s 0.9–1.6 ms. Frames over 1.2 ms
  (~0.5%) are real CPU time, not preemption. They are busy frames
  (SMG bots ~0.45, entity thinks ~0.25, sends ~0.28, end-frame ~0.09 ms)
  plus a respawn (~0.19 ms each: weapon `setModel`/init commands 0.06,
  `LoadStateTable` 0.04, `ChooseSpawnPoint` 0.025) or a weapon drop.
  SMG bot think is ~0.011 ms per bot (pmove 0.0045, steering 0.0036,
  nav 0.0013, enemy 0.001). Remaining exact ideas are each ≤0.04 ms.
- Profiling note: Release builds have no frame pointers, so gperftools
  call stacks misattribute (SMG-bot work showed up under roombas). Trust
  self time, or time code directly.

### Audit leftovers (2026-10-25)

The owner's two audit lists were checked against the tree; almost all
items were already done. The rest, measured on the live-like bench:

- Kept: `-fno-semantic-interposition` for GCC builds of the game module
  (`cmake/basegame.cmake`). 3 pairs: average 0.58 -> 0.55 ms, p99 0.96 ->
  0.88 ms, frames over 1.2 ms 22 -> 5. `GetGameAPI` stays exported.
- Kept: "Server frame overran: N ms" in the server log next to `svlag`
  (already once per 2.5 s); not exercised at runtime (no overrun occurred).
- Kept, exact: `CheckEndMatch` re-fetches `fraglimit`/`timelimit` only
  after clamping; one shared `dtQueryFilter` per ladder setting for path
  searches (checked byte for byte through a fight); the parsed BSP
  triangles are freed after a navmesh build outside developer mode.
- Not kept: sleeping the full frame wait instead of spinning the last
  millisecond. Idle CPU 1.89% -> 0.08% of a core, but in fights the core
  cools between frames: 6 pairs averaged 0.66 -> 0.69 ms and had more
  frames over 1.2 ms. Owner to decide; reverted for now.
- Not changed: SMG-bot jump/edge probes cost ~0.027 ms/frame for 20 bots
  (a 100 ms cadence would save ~0.013), and the 100 ms stuck retry fires
  0-7 times per 10 s; both below the bar for a timing change.
- Kept, exact: snapshot entity pre-pass (`sv_snapshot.c`). The first
  snapshot in each `SV_SendClientMessages` pass lists the entities that pass
  the client-independent checks (and sets `SVF_SENT` as the full loop
  would); later clients walk that list (191 of 339 entities on mohdm6).
  Other callers and a cut-short pass (`SV_SnapshotListReset` in
  `SV_Shutdown`/`SV_SpawnServer`) use the full loop. Checked against a fresh
  recompute 80,000 times; 3 pairs: p99 1.24 -> 1.21 ms, frames over 1.2 ms
  62 -> 47, average unchanged.
- Skipped by owner or rule: `-march`, far-sound culling, `_IOLBF` (a log
  line is already one write).
- Ops advice (no code): `sv_minRate`/`sv_maxRate` 25000+, `logfile 1`,
  `com_speeds 1` briefly to measure, `TZ=:/etc/localtime`, restart before
  23 days of uptime.

### Review fixes (2026-10-25, folded into the commits they correct)

A full review of the 10 commits not in `upstream/main` found 11 issues; the
owner asked for 1-7, with 2 evaluated first:

1. Region labels are now a union-find over every navmesh link taken as
   undirected (fall/high-ledge off-mesh links are one-way). The old forward
   flood skipped 0 reachable candidates in 400 skips on mohdm6/mohdm2, so it
   was latent; the check build confirmed every new skip is unreachable.
2. FFA score cache kept: `score` requests come at most every 2 s per client
   and only between frames, so a reused string is at most one 50 ms frame
   older than a fresh one. Comment corrected; no code change.
3. GameSpy queries have their own rate-limit bucket pool (`queryPool`,
   `SVC_RateLimitQueryAddress`), global cap 30 burst then 20/s. Tested:
   before, a 50-query burst from one IP made the server drop that IP's
   `getchallenge`; now it is answered.
4. Minimum travel distance for SMG-bot moves is again checked on the final
   destination in `AvoidPath`/`Wander` (any pather) and on `closestPos` in
   the Recast searches.
5. Footstep path restored except the content/sound-name work (see above).
6. "population fill needs" printed once per change (was ~97 lines per fill).
7. Navmesh cache header: map name end found with `memchr` inside the field;
   two corrupt-header cases added to `test_navmesh_cache`.

RNG: the stream matches stock except where SMG-bot behavior was changed on
purpose (wander, pacing), i.e. only when SMG bots are enabled.

## Remaining

Audit recommendations that need an owner decision or game assets:

- `sp_ffa`: `removeclass Trigger NIL 1` also removes items, script objects and
  models, doors (their area portals stay closed) and speakers. `m4l3` also
  removes its doors explicitly, so fixing only that line will not reopen its
  portals.
- `m6l2a`: a kept `trigger_changelevel` could end the match; `cg_rain_*`
  cvars never reach clients; neither pack precaches `dm_50_healthbox.tik`; the
  package holds CRLF copies of the scripts, so it cannot be rebuilt
  byte-for-byte from source; the "dm" sound-alias rule applies on every
  multiplayer map, not only single-player ones as documented.
- SP taunts: if the stock taunt aliases are streamed, their first use on an
  SP map adds a sound configstring mid-game (confirm with the game files).
- SMG bots: fire while still turning. The navmesh is now cached on disk,
  but still built on a cache miss.
- Roombas: a visible but unreachable target locks them; obituary lines are
  no longer logged; names shift after a bot is kicked; `sv_maxclients` cannot
  be lowered while bots hold top slots.
- Names: "Your name is now" never reaches a player renamed on joining (it is
  sent before the client can receive commands); a userinfo padded close to
  1350 bytes loses its name when the game writes back a fixed name, leaving
  a blank server-side name.
- Spectating: `join_team` from the ESC menu still escapes a death through the
  upstream respawn path; only `g_teamswitchdelay` limits it.
- Rotation: `autoexec.cfg` runs before the commands exist; a missing map at
  load drops the server; `maprotation_advance_internal` can be run by hand.
- Chat bans: reads can come from another XDG home path; saves are not atomic
  and drop `//` comment lines and rejected records; a UTF-8 BOM makes the
  first record invalid; `callvote` text still broadcasts; `::ffff:` entries
  never match.
- Admin: ban commands always report success; `ad_banip` wildcards widen
  silently; `ad_banip`, `ad_banipr`, `ad_banipsilent` and `ad_unbanip` accept
  bare numbers, which the engine treats as a slot or ban index; admin login
  names are broadcast.
- Docs: the compiling guide still lists SDL2/OpenAL; some new files have short
  license headers.

## Blockers / unknowns

- CI compiles and passes unit tests, but the Builds workflow fails at
  `upload-artifact` because the account's Actions artifact storage quota is
  full, so no `out-linux-amd64` package is produced until old artifacts are
  deleted or usage is recalculated. CodeQL fails because code scanning is not
  enabled in the repository settings (since 2026-08-31, before this work).
- This environment's git access refuses tag pushes and ref deletions (HTTP
  403); pushes to branches, including force-pushes to `main`, work. Auto mode
  also blocks amending a commit that is already pushed.
- The SP-map findings need the map files to confirm.
- The daily 02:55 UTC VM pause needs the owner to check the VPS provider
  panel (automated backups or snapshots) or open a ticket.
- What fills the per-client CGM buffer mid-game is not known yet.
- The guard's resources tick (`MOHARENA_GUARD_RESOURCES_TICK_INTERVAL`,
  default 5 s) sends status, serverinfo and 7 cvar reads each time; now
  that they are answered it is the guard's biggest rcon load. Raising it
  waits for the owner.

## Verified earlier (committed history)

- Debug and RelWithDebInfo builds; CTest (LZ77, chat bans) passes.
- Dedicated-server smoke tests: startup, chat-ban path refusals, rotation
  start holding on the lock map, no DNS traffic for rejected addresses.
- ASan/UBSan harnesses: chat-ban load/write refusals, name-change scenarios,
  the connect-userinfo overflow repro, and old-versus-new SMG jump checks
  (identical results on 240,000 random states).
- The game module loads under the API 17 handshake; SMG bot behavior itself
  was not run without map assets.
- Second audit: seven reviewers read every commit after `a2f3401` at its own
  point in history, and each finding was re-checked against the code. The
  final code differs from the first rewrite only by the four second-audit
  fixes.
- A harness replaying the rename path kicks the padded-userinfo client on its
  second rename, and never kicks for game-forced names or settings changes.
- Every commit passes a clean GCC Debug build and CTest; tests are registered
  only from the chat-ban commit on.

- Do not commit the untracked `.cmake/` (a local build directory) or
  `.cursor/` unless the owner wants them.

## Verified

- Out-of-tree GCC Debug build (server + game module) succeeds; CTest 4/4.
- Fight-load pass: a check build ran old and new logic side by side for
  several minutes of a 20+20 bot fight on mohdm6 and aborted on none
  (index cache, delta early-out, field compare, Huffman bytes, leaf cache).
- Pass 2: a check build ran 5 minutes of the fight with every delta write
  compared against a direct encode (8 million), plus Huffman bytes, chunked
  bits, snapshot sort and obstacle validity; no differences. CTest 4/4.
- Smoke test on `dm/mohdm6` with stock Pak files: clean start and shutdown;
  first load writes the navmesh cache, second load hits it (0.65 s → 0.03 s).
- Review fixes: check build confirmed every region skip unreachable and
  every accepted SMG-bot move past its minimum; live UDP test showed a
  50-query GameSpy burst no longer blocks that IP's `getchallenge`; the
  bot-fill warning prints once (was ~97 times). CTest 4/4.
- Owner played a local mohdm6 session with 5 roombas and 5 SMG bots
  (2026-10-25) and reported nothing wrong. Not run: the live server.

## Exact next action

After a full evening on `e41c1a0`, compare player stall episodes (group
"Delta request from out of date" lines per player, 2 s apart) and the
ring coverage at 20+ humans with 2026-10-29 (0.63 episodes/min, 0.22 with
a ring reset). If mid-game frame spikes over 40 ms recur, find what that
tick ran.
