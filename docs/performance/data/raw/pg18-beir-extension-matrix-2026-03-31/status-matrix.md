# Matrix Status

Updated: `2026-03-31T15:15:57.864494+00:00`

Status counts: `{'done': 75}`

| Dataset | upstream | psql_ids | psql_text[] | pg_search | vchord_bm25 |
|---|---|---|---|---|---|
| `arguana` | `done 1.21s` | `done 1.23s` | `done 1.28s` | `done 12.13s` | `done 17.85s` |
| `climate-fever` | `done 8.4m` | `done 38.70s` | `done 40.11s` | `done 9.0m` | `done 4.9m` |
| `cqadupstack` | `done 2.0m` | `done 40.92s` | `done 56.20s` | `done 15.7m` | `done 3.6m` |
| `dbpedia-entity` | `done 2.2m` | `done 5.27s` | `done 7.04s` | `done 1.5m` | `done 19.74s` |
| `fever` | `done 651.8m` | `done 30.2m` | `done 28.5m` | `done 364.9m` | `done 169.1m` |
| `fiqa` | `done 8.20s` | `done 6.75s` | `done 7.38s` | `done 6.2m` | `done 34.88s` |
| `hotpotqa` | `done 391.9m` | `done 28.8m` | `done 34.3m` | `done 476.4m` | `done 175.2m` |
| `msmarco` | `done 5284.1m` | `done 142.3m` | `done 182.6m` | `done 1916.2m` | `done 467.0m` |
| `nfcorpus` | `done 1.03s` | `done 957ms` | `done 1.00s` | `done 2.86s` | `done 2.58s` |
| `nq` | `done 5.5m` | `done 29.77s` | `done 29.51s` | `done 9.2m` | `done 2.6m` |
| `quora` | `done 2.8m` | `done 33.46s` | `done 33.69s` | `done 18.9m` | `done 1.6m` |
| `scidocs` | `done 831ms` | `done 728ms` | `done 698ms` | `done 55.90s` | `done 2.72s` |
| `scifact` | `done 374ms` | `done 503ms` | `done 508ms` | `done 2.22s` | `done 1.76s` |
| `trec-covid` | `done 238ms` | `done 311ms` | `done 376ms` | `done 5.71s` | `done 661ms` |
| `webis-touche2020` | `done 204ms` | `done 646ms` | `done 703ms` | `done 6.02s` | `done 570ms` |

Legend: `creating` create requested but VM not present yet, `boot` VM booting, `wait` waiting, `staged` assets uploaded but bootstrap not started, `install` OS/extension install, `setup` Python/bootstrap setup, `tokenize` tokenize, `load` DB load, `build` index build, `query` query phase, `run` active but phase not inferred, `done Xm/Xs/Xms` local result saved with total query time, `done(vm)` finished on VM but not yet recovered locally, `error` result JSON saved but task failed, `ssh err` probe failure, `gone` launch said running but VM missing.
