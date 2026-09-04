# M1554 Sharded P1 Protected-Tail Report

## Question

M1553 found a strong token-aware lexical-residual signal on nine official
full-corpus rows served by one model-backed P1 index regclass.  M1554 asks
whether the same frozen-top99 mechanism transfers to the three available
multi-million-document P1 generation-shard surfaces.

No model, teacher, threshold, or dataset-specific parameter is changed.  The
run reuses the complete climate-fever, dbpedia-entity, and NQ native generation
shards, adds at most 16 native BM25 candidates, and changes only rank 100.
Qrels are read after candidate generation and ordering.

## Full Giant3 Result

The full run covers 5,387 queries and completed in 2,197 seconds.

| Dataset | P1 R@100 | Protected R@100 | Delta | CUB delta | MAP delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| climate-fever | 0.573366 | 0.581173 | +0.007807 | +0.025841 | +0.000134 |
| dbpedia-entity | 0.332024 | 0.359900 | +0.027876 | +0.112512 | +0.000992 |
| nq | 0.774913 | 0.804196 | +0.029283 | +0.091058 | +0.000332 |
| **Macro** | **0.560101** | **0.581756** | **+0.021655** | **+0.076470** | **+0.000486** |

NDCG@10 and MRR@20 are unchanged on every row by construction.  All three
dataset rows pass the quality gate.  Query-level actions still contain a small
number of harms: 4 on climate-fever, 4 on dbpedia-entity, and 2 on NQ.  The
positive row aggregate therefore does not by itself prove a safe selector.

## Absolute Dense Boundary

The residual gain must not be confused with dense equivalence.  On the same
three official rows, the external native VectorChord dense and M1549 results
remain materially stronger:

| Variant | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| VectorChord dense | 0.455552 | 0.360955 | 0.691679 | 0.595795 | 0.746056 |
| M1549 dense + lexical | 0.455552 | 0.361269 | 0.701710 | 0.595795 | 0.785073 |
| P1 semantic + protected lexical | 0.367896 | 0.280683 | 0.581756 | 0.508207 | 0.683051 |

The unified P1 route remains below dense by `0.109923` Recall and `0.087656`
NDCG on this surface.  M1554 therefore validates lexical residual capacity,
not the first-stage semantic compiler.

## Proof Boundary

The official9 M1553 evidence uses one P1 index regclass for semantic and
lexical candidates.  The giant3 backend must merge existing semantic
generation shards and query the native BM25 base index separately because the
single-generation publisher cannot materialize these corpora.  M1554 is thus
cross-corpus capacity and ranking evidence, not a claim that the giant3
physical one-index lifecycle is closed.

## Decision

**Promote M1554 as residual-mechanism evidence.**  Keep the deterministic
token-aware lexical head and hard top99 controller as the only supported
residual shape.  Do not promote the route as a dense replacement and do not
start joint/LoRA training: the frozen pooled residual head already failed in
M1552, while the remaining absolute gap is semantic candidate access.

Proceed once to M1556 action observability.  If gain/harm is not separable on
held-out corpora, stop learned guard work and retain M1549's native ANN plus
lexical architecture as the recall product baseline.
