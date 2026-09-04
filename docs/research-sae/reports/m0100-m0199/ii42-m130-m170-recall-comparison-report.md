# ii42 M130-M170 Recall Comparison Report

Date: 2026-06-10

## Scope

This report collects the best known recall-oriented results from the M130,
M150, M160, and M170 model lines.

There are two different evaluation surfaces in this file:

- **Continuity full-corpus surface**: the recurring `993,336` document /
  `886` query PPLX all-data evaluation surface used for M130/M150/M160
  continuity comparisons.
- **Official BEIR partial per-dataset surface**: completed official BEIR
  full-corpus dataset gates. This is broader and more realistic, but currently
  incomplete for the largest datasets.

Do not compare numbers across these surfaces as if they were the same test.

## Best Comparable Continuity Results

| Series | Best available version / profile | BM25 R@100 | Dense R@100 | BM25+dense R@100 | SAE-only R@100 | BM25+SAE R@100 | Status |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| M130 | Stage C default `doc96/query96` | 0.2439 | 0.3132 | 0.3154 | 0.3394 | 0.3388 | Comparable continuity gate |
| M130 | Stage C `doc64/query80` | 0.2439 | 0.3132 | 0.3154 | 0.3302 | 0.3349 | Production-shaped lower-cost profile |
| M150 | A1 C6 official dense-miss | 0.2439 | 0.3132 | 0.3161 | 0.3899 | 0.3841 | Strongest known continuity result |
| M150 | A1 C6 post-hoc best recall | 0.2439 | 0.3132 | 0.3161 | 0.3899 | 0.3928 | Diagnostic only; not a learned deployment row |
| M160 | M160A C6 official dense-miss | 0.2439 | 0.3132 | 0.3152 | 0.3279 | 0.3263 | Comparable but weaker than M130/M150 |
| M160 | M160A C6 post-hoc best recall | 0.2439 | 0.3132 | 0.3152 | 0.3279 | 0.3370 | Diagnostic fixed profile |
| M170 | M170A broad M150-loss | n/a | n/a | n/a | n/a | n/a | No comparable full-corpus gate found |

## Full Metric Table For Best Continuity Rows

| Series | Version / row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| Baseline | BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Baseline | Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| Baseline | BM25+dense score fusion | 0.3152-0.3161 | 0.2815-0.2897 | 0.2086-0.2153 | 0.1375-0.1421 |
| M130 | Stage C default SAE-only | 0.3394 | 0.3653 | 0.2570 | 0.1642 |
| M130 | Stage C default BM25+SAE | 0.3388 | 0.3648 | 0.2490 | 0.1583 |
| M130 | Stage C `doc64/query80` BM25+SAE | 0.3349 | 0.3688 | 0.2521 | 0.1626 |
| M150 | A1 C6 SAE-only | 0.3899 | 0.4363 | 0.3120 | 0.2107 |
| M150 | A1 C6 learned BM25+SAE | 0.3841 | 0.4271 | 0.3010 | 0.2008 |
| M150 | A1 C6 post-hoc best recall | 0.3928 | n/a | n/a | n/a |
| M160 | C6 SAE-only | 0.3279 | 0.2922 | 0.2217 | 0.1541 |
| M160 | C6 learned BM25+SAE | 0.3263 | 0.2966 | 0.2162 | 0.1439 |
| M160 | C6 post-hoc balanced fixed profile | 0.3339 | 0.2916 | 0.2267 | 0.1581 |
| M160 | C6 post-hoc best recall profile | 0.3370 | 0.2897 | 0.2242 | 0.1576 |

## M150 A1 C6 Per-Dataset Continuity Signals

This is not the complete BEIR15 gate. It is a per-dataset breakdown available
from the M150 A1 C6 continuity diagnostic.

| Dataset | Dense R@100 | SAE-only R@100 | BM25+SAE R@100 | Main signal |
| --- | ---: | ---: | ---: | --- |
| arguana | 1.0000 | 1.0000 | 1.0000 | Saturated; fusion choice is mostly ranking-only |
| fiqa | 0.7792 | 0.6161 | 0.5821 | Representation gap remains; BM25 hurts |
| msmarco | 0.7349 | 0.6862 | 0.7297 | BM25 helps recall, ranking remains sensitive |
| nfcorpus | 0.1925 | 0.2035 | 0.2069 | BM25+SAE is slightly useful |
| scifact | 0.8000 | 0.9333 | 1.0000 | BM25+SAE can help strongly |
| trec-covid | 0.2599 | 0.2124 | 0.2579 | BM25 recovers recall, ranking is noisy |

## M160A C6 Official BEIR Partial Per-Dataset Signals

This is the completed official BEIR full-corpus partial gate. It covers nine
datasets. The following large datasets were still missing from that partial
matrix: `nq`, `dbpedia-entity`, `hotpotqa`, `fever`, `climate-fever`, and
`msmarco`.

| Dataset | Dense R@100 | SAE-only R@100 | BM25+SAE R@100 | BM25+SAE union R@100 | Main signal |
| --- | ---: | ---: | ---: | ---: | --- |
| arguana | 1.0000 | 0.9893 | 0.9936 | 0.9950 | Near saturated |
| cqadupstack | 0.7826 | 0.7211 | 0.7222 | 0.7624 | Candidate headroom exists, ranking/admission weak |
| fiqa | 0.8290 | 0.7659 | 0.7583 | 0.8011 | BM25 fusion hurts SAE positives |
| nfcorpus | 0.3270 | 0.3023 | 0.3140 | 0.3475 | BM25 adds useful recall headroom |
| quora | 0.9960 | 0.9933 | 0.9956 | 0.9973 | Near saturated |
| scidocs | 0.4958 | 0.4634 | 0.4549 | 0.5056 | Fusion loses SAE positives; union has headroom |
| scifact | 0.9633 | 0.9450 | 0.9767 | 0.9833 | BM25 admission helps strongly |
| trec-covid | 0.1673 | 0.1286 | 0.1301 | 0.1858 | Large union headroom, ranking remains hard |
| webis-touche2020 | 0.4928 | 0.4832 | 0.5717 | 0.6534 | BM25 admission is very valuable |

M160A C6 official partial aggregate:

| Source | R@20 | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.4518 | 0.5625 | 0.4894 | 0.4195 | 0.2849 |
| Dense | 0.5520 | 0.6726 | 0.5999 | 0.5357 | 0.3748 |
| BM25+dense score fusion | 0.5562 | 0.6734 | 0.6010 | 0.5342 | 0.3716 |
| SAE-only | 0.5220 | 0.6436 | 0.5483 | 0.4791 | 0.3382 |
| BM25+SAE score fusion | 0.5393 | 0.6574 | 0.5764 | 0.5055 | 0.3573 |

## M170 Available Readout

M170 does not have a comparable full-corpus Recall@100 gate in the local report
set or on Spark-1. The available result is the M170A Stage-A candidate-surface
summary.

| Series | Version | Surface | BM25 Hit@20 | Dense Hit@20 | Model Hit@20 | BM25 MRR@20 | Dense MRR@20 | Model MRR@20 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M170 | `ii42-m170a-broad-m150loss-v1` | Candidate surface, 886 rows | 0.6061 | 0.6817 | 0.8736 | 0.3951 | 0.4677 | 0.6878 |

This is useful as a Stage-A representation signal, but it must not be promoted
or compared directly with M130/M150/M160 full-corpus Recall@100 rows.

## Interpretation

- **M150 A1 C6 is the strongest known continuity-surface model** by both
  SAE-only and BM25+SAE Recall@100.
- **M130 Stage C remains the clean older baseline** and has a lower-cost
  production-shaped profile at `doc64/query80`.
- **M160A C6 has the best official partial BEIR breakdown**, but it still
  trails dense/BM25+dense on aggregate official partial metrics.
- **M160A union recall shows real headroom**. The gap is often not candidate
  existence, but admission and top-rank scoring.
- **M170A is not a full-corpus result**. It should be treated as a Stage-A
  candidate-surface signal only.

## Source Reports

- `docs/research-sae/reports/m0100-m0199/sae-m130-bm25-sae-results-report.md`
- `docs/research-sae/reports/m0100-m0199/sae-m150-stage-b-from-best-results-report.md`
- `docs/research-sae/reports/m0100-m0199/sae-m150-c6-posthoc-fusion-diagnostic-report.md`
- `docs/research-sae/reports/m0100-m0199/sae-m150-c7-official-representative-gate-report.md`
- `docs/research-sae/reports/m0100-m0199/ii42-m160-b8-full-corpus-comparison-report.md`
- `docs/research-sae/reports/m0100-m0199/ii42-m160a-c6-posthoc-fusion-diagnostic-report.md`
- `docs/research-sae/reports/m0100-m0199/ii42-m160a-admission-fusion-diagnostic-report.md`
- `docs/research-sae/reports/m0100-m0199/ii42-m160a-stage-c-admission-ranker-report.md`
- `docs/research-sae/reports/m0100-m0199/ii42-m170-m160-rigor-m150-c6-plan.md`
- `docs/research-sae/reports/m0100-m0199/ii42-m180-clean-stage-a-plan.md`
