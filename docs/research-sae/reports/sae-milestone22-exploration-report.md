# SAE Milestone 22 Exploration Report

Date: 2026-05-15

## Summary

M22 is promising, but the current evidence says to keep the Snowflake-SAE teacher/student path as the quality baseline while building a paper-aligned token-level concept encoder as a controlled next experiment.

The useful result from the first pass was that the evaluation gates became concrete: current quality, off-the-shelf SPLADE control, sparse physical cost, PostgreSQL smoke status, and concept diagnostics are in one report. A follow-up token-level training pass now adds a real model signal: `token_lse + candidate-budget` can approach the teacher on ranking metrics at active 128/192, but physical sparse cost is still unproven.

## Validation Commands

```bash
python3 scripts/test_research_sae_unified_payload_pg.py \
    --temp-postgres \
    --library ./ii42.dylib
python3 scripts/test_research_sae_evidence_atom_pg_generation.py \
    --temp-postgres \
    --library ./ii42.dylib
PYTHONPATH=scripts python3 scripts/research_sae_m22_exploration.py
```

Both PostgreSQL smoke tests passed before this report was generated.

## Full15 Current Quality Baseline

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | --- | --- | --- |
| `bm25` | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| `bm25_teacher_sae` | 0.8425 | 0.8458 | 0.7507 | 0.7249 |
| `bm25_student_atoms_w0p5` | 0.8280 | 0.8292 | 0.7223 | 0.6931 |
| `bm25_student_atoms` | 0.8324 | 0.8291 | 0.7225 | 0.6913 |
| `bm25_student_atoms_w1` | 0.8340 | 0.8240 | 0.7142 | 0.6782 |

## Five-Dataset SPLADE Control

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean ms |
| --- | --- | --- | --- | --- | --- |
| `bm25` | 0.7035 | 0.5904 | 0.5031 | 0.4134 | 5.5364 |
| `bm25_sae` | 0.7934 | 0.6742 | 0.5952 | 0.4987 | 5.6762 |
| `splade` | 0.7519 | 0.6324 | 0.5559 | 0.4615 | 6.3750 |
| `bm25_splade` | 0.7431 | 0.6399 | 0.5539 | 0.4572 | 7.9785 |
| `bm25_sae_splade` | 0.7899 | 0.6814 | 0.6032 | 0.5043 | 7.6224 |

Interpretation: ordinary SPLADE is a useful negative/control result. It beats pure BM25 on this subset, but it does not beat BM25+SAE, so M22 should train a TopK SAE concept-vocabulary encoder instead of promoting this checkpoint into SQL.

## UBMX Sparse Cost Metrics

| Metric | Value |
| --- | --- |
| Exact match rate | 1.0000 |
| Recall@100 | 0.8286 |
| NDCG@10 | 0.7250 |
| MAP@100 | 0.6880 |
| Mean query terms | 17.2680 |
| Mean SAE postings | 787.4347 |
| Mean BM25 postings | 381.4140 |
| Estimated QD-FLOPs proxy | 1168.8487 |
| Mean candidate docs | 738.4567 |
| Mean rerank score terms | 5540.6773 |
| Sparse interactions with rerank | 6709.5260 |
| Payload MB | 6.6398 |
| Memory MB | 6.6408 |

The QD-FLOPs number here is a project-local proxy: SAE postings plus BM25 postings touched during candidate generation. It is reported alongside actual candidate docs and exact rerank terms so model gains cannot hide behind unbounded fanout.

## Gate Status

| Gate | Status | Evidence |
| --- | --- | --- |
| M22.1 sparse cost metrics | pass | UBMX C benchmark exposes query terms, postings, candidate docs, rerank terms, and payload memory. |
| M22.2 SAE-SPLADE control | partial pass | `token_max` failed, but `token_lse + candidate-budget` reached useful ranking quality at active 128/192. See [SAE Milestone 22 Token-Level Concept Training Report](sae-milestone22-token-level-training-report.md). |
| M22.3 current teacher/student comparison | pass | student recall lift over BM25 is 0.0487; gap to teacher is 0.0101 Recall@100 and 0.0168 MRR@20. |
| M22.4 larger latent vocabularies | open | No new 16k/32k/64k TopK concept-vocabulary model was trained in this pass; require M22.2 first. |
| M22.5 PostgreSQL runtime query shape | partial | Unified payload bytea/by-id smoke passed; lower-level evidence-atom runtime query smoke passed. A product-shaped unified BM25+concept query_atoms SQL function is still open. |
| M22.6 concept diagnostics | pass | Initial atom DF and top-token diagnostics generated for the five-dataset control. |
| M22.7 policy multilingual probe | open | No local policy sample was available in this pass; keep as unlabeled diagnostic after a concept encoder exists. |

## Initial Concept Diagnostics

These diagnostics use current text-student atoms, not a new SAE-SPLADE encoder. They are still useful because the same reporting surface should be required for any future concept vocabulary.

### `scifact`

- documents: `2000`
- doc active mean/p95: `64.0` / `64.0`
- query active mean/p95: `64.0` / `64.0`
- unique doc atoms: `6673`

| Atom | DF | DF Ratio | Avg Weight | Top Tokens |
| --- | --- | --- | --- | --- |
| 5638 | 911 | 0.4555 | 0.3121 | `cells`, `cell`, `protein`, `expression`, `results`, `role`, `show`, `gene` |
| 1496 | 666 | 0.3330 | 0.5016 | `cells`, `cell`, `protein`, `expression`, `gene`, `results`, `human`, `show` |
| 3452 | 621 | 0.3105 | 0.6223 | `results`, `associated`, `study`, `increased`, `may`, `disease`, `patients`, `risk` |
| 8147 | 610 | 0.3050 | 0.0846 | `cells`, `cell`, `expression`, `show`, `role`, `induced`, `protein`, `activation` |
| 4885 | 591 | 0.2955 | 0.1075 | `cells`, `cell`, `expression`, `protein`, `show`, `results`, `role`, `induced` |
| 2819 | 590 | 0.2950 | 0.4333 | `cells`, `results`, `cell`, `expression`, `treatment`, `study`, `may`, `role` |
| 6231 | 523 | 0.2615 | 0.0604 | `cells`, `cell`, `expression`, `results`, `role`, `induced`, `protein`, `associated` |
| 2880 | 467 | 0.2335 | 0.3638 | `cells`, `cell`, `results`, `expression`, `protein`, `show`, `activation`, `role` |

### `scidocs`

- documents: `2000`
- doc active mean/p95: `64.0` / `64.0`
- query active mean/p95: `64.0` / `64.0`
- unique doc atoms: `6602`

| Atom | DF | DF Ratio | Avg Weight | Top Tokens |
| --- | --- | --- | --- | --- |
| 8177 | 890 | 0.4450 | 0.5401 | `paper`, `based`, `using`, `data`, `results`, `approach`, `used`, `information` |
| 3899 | 859 | 0.4295 | 0.2258 | `paper`, `based`, `using`, `data`, `results`, `approach`, `system`, `performance` |
| 4984 | 596 | 0.2980 | 0.4238 | `paper`, `based`, `results`, `using`, `data`, `proposed`, `method`, `used` |
| 2376 | 595 | 0.2975 | 0.2922 | `paper`, `based`, `network`, `using`, `networks`, `data`, `performance`, `results` |
| 2279 | 566 | 0.2830 | 0.8187 | `paper`, `based`, `data`, `using`, `results`, `information`, `performance`, `use` |
| 4848 | 541 | 0.2705 | 0.1082 | `based`, `paper`, `using`, `learning`, `data`, `results`, `performance`, `method` |
| 4489 | 537 | 0.2685 | 0.4469 | `learning`, `based`, `paper`, `data`, `using`, `model`, `results`, `approach` |
| 766 | 515 | 0.2575 | 0.4673 | `paper`, `based`, `data`, `using`, `results`, `approach`, `show`, `problem` |

### `nfcorpus`

- documents: `2063`
- doc active mean/p95: `64.0` / `64.0`
- query active mean/p95: `64.0` / `64.0`
- unique doc atoms: `6362`

| Atom | DF | DF Ratio | Avg Weight | Top Tokens |
| --- | --- | --- | --- | --- |
| 3452 | 1371 | 0.6646 | 0.5239 | `study`, `results`, `risk`, `may`, `associated`, `dietary`, `diet`, `intake` |
| 730 | 1234 | 0.5982 | 0.2007 | `study`, `results`, `dietary`, `diet`, `may`, `intake`, `risk`, `associated` |
| 1709 | 837 | 0.4057 | 0.3851 | `results`, `study`, `risk`, `associated`, `may`, `methods`, `intake`, `years` |
| 1594 | 792 | 0.3839 | 0.7057 | `study`, `results`, `diet`, `dietary`, `risk`, `intake`, `associated`, `may` |
| 7015 | 778 | 0.3771 | 0.3818 | `study`, `results`, `diet`, `dietary`, `intake`, `may`, `high`, `associated` |
| 1080 | 751 | 0.3640 | 0.5363 | `study`, `dietary`, `results`, `intake`, `diet`, `risk`, `associated`, `may` |
| 7688 | 697 | 0.3379 | 0.1074 | `study`, `results`, `risk`, `associated`, `intake`, `dietary`, `methods`, `may` |
| 1231 | 697 | 0.3379 | 0.5688 | `study`, `results`, `risk`, `may`, `food`, `intake`, `dietary`, `associated` |

### `arguana`

- documents: `2000`
- doc active mean/p95: `64.0` / `64.0`
- query active mean/p95: `64.0` / `64.0`
- unique doc atoms: `5548`

| Atom | DF | DF Ratio | Avg Weight | Top Tokens |
| --- | --- | --- | --- | --- |
| 6440 | 1571 | 0.7855 | 0.6895 | `house`, `people`, `government`, `may`, `even`, `states`, `them`, `one` |
| 4892 | 1155 | 0.5775 | 0.8456 | `house`, `international`, `people`, `states`, `may`, `government`, `state`, `even` |
| 2254 | 1120 | 0.5600 | 0.7990 | `house`, `people`, `government`, `may`, `even`, `one`, `state`, `states` |
| 5887 | 938 | 0.4690 | 0.5450 | `states`, `international`, `house`, `state`, `government`, `people`, `countries`, `may` |
| 3362 | 901 | 0.4505 | 0.5144 | `people`, `state`, `government`, `house`, `states`, `may`, `one`, `them` |
| 4881 | 897 | 0.4485 | 0.7217 | `people`, `house`, `states`, `even`, `state`, `them`, `may`, `government` |
| 3326 | 850 | 0.4250 | 0.3511 | `house`, `states`, `international`, `them`, `even`, `people`, `state`, `many` |
| 15 | 782 | 0.3910 | 0.7055 | `states`, `state`, `people`, `government`, `may`, `international`, `house`, `even` |

### `fiqa`

- documents: `2000`
- doc active mean/p95: `64.0` / `64.0`
- query active mean/p95: `64.0` / `64.0`
- unique doc atoms: `6220`

| Atom | DF | DF Ratio | Avg Weight | Top Tokens |
| --- | --- | --- | --- | --- |
| 6386 | 1046 | 0.5230 | 0.1119 | `money`, `some`, `one`, `get`, `don`, `like`, `make`, `market` |
| 6440 | 927 | 0.4635 | 0.6141 | `money`, `people`, `get`, `like`, `some`, `don`, `make`, `one` |
| 6002 | 759 | 0.3795 | 0.3367 | `money`, `get`, `pay`, `some`, `one`, `don`, `much`, `make` |
| 4641 | 726 | 0.3630 | 0.3613 | `money`, `get`, `pay`, `one`, `tax`, `some`, `don`, `may` |
| 4138 | 696 | 0.3480 | 0.1727 | `get`, `don`, `money`, `like`, `some`, `make`, `people`, `one` |
| 7880 | 683 | 0.3415 | 0.1043 | `money`, `get`, `don`, `one`, `some`, `pay`, `like`, `bank` |
| 6004 | 670 | 0.3350 | 0.4794 | `get`, `like`, `don`, `some`, `money`, `one`, `people`, `much` |
| 4984 | 668 | 0.3340 | 0.5801 | `money`, `get`, `don`, `some`, `like`, `time`, `one`, `make` |

## Decision

Proceed with asymmetric active-budget training and physical-cost evaluation for the M22 `token_lse` checkpoint. Do not promote ordinary SPLADE, and do not move the token-level model into SQL until it proves that active 128/192 quality can be reached with bounded query fanout.

The SQL side should stay read-only for now. The lower-level runtime atom path works, but the unified BM25+concept payload still needs a single product-shaped `query_atoms` function before API design should be frozen.
