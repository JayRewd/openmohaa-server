# Current State

Reconciled: 2026-10-25

## Active task

Server optimization pass that stock-client players must not notice.
Reviewed and finalized in the working tree; **uncommitted on purpose** (the
owner commits it). The packet-demo feature (D-011) is planned only; no code.

## Why this serves the goal

Removes recurring server work (navmesh rebuild at every map load, event-queue
scans, per-client score/top-score rebuilds, idle footstep/AI-event traces,
query floods) without changing what clients receive.

## Status

`main` equals `origin/main` and `upstream/main` (`f680c87`). Remotes:
`origin` = `JayRewd/openmohaa-server` (fork, push target); `upstream` =
`pstngh/openmohaa-server` (PR base). Everything below is uncommitted.

### In the tree (kept)

- Navmesh disk cache `navmesh/<map>.bin` (homepath), keyed by BSP length +
  MD4 + `NAVMESH_CACHE_VERSION`; skipped with `developer`. Unit test
  `test_navmesh_cache`. Bump the version on any nav build change.
- Nav region labeling; `FindPathAway` skips unreachable islands; SMG-bot idle
  wander uses `FindPathNear` within its region (roaming looks the same).
- Per-Listener pending-event count short-circuits cancel/pending scans.
- State machines: resolved next-state pointers and pretokenized commands.
- FFA: score string cached per frame; top score computed once per frame.
- Footstep/AI-event work skipped when no Actor exists and `sv_netoptimize`
  is off (its only outputs feed Actors and that cvar's sounds).
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

### Fight-load pass (2026-10-25, uncommitted)

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

### Fight-load pass 2 (2026-10-25, uncommitted)

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
- SMG bots: fire while still turning. The navmesh is now cached on disk
  (uncommitted), but still built on a cache miss.
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
- The authoritative live game server is not identified; deployment is outside
  this change.
- The SP-map findings need the map files to confirm.

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
- Not run: SMG bots with humans connected, live stock-client session.

## Exact next action

Owner reviews `git diff` and commits to `origin` (the JayRewd fork), then
opens a PR to `upstream` (`pstngh/openmohaa-server`) if desired. Then test on
the live server with a stock client: join, map change, explosions, bots
roaming and jumping.
