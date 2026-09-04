# II-42 M367 BEIR15 Dense Coordinate Posting Report

## Goal

M367 moves the M366 dense-coordinate posting baseline from the M80 local gate to
the local shared BEIR15 artifact face.

The question is deliberately dense-only:

- no BM25;
- no qrel training;
- no learned SAE atoms;
- no admission scorer;
- no reranker.

It asks whether signed dense-coordinate postings can preserve dense retrieval
quality across BEIR15-style datasets, and what fanout cost this preservation
requires.

## Scope

Input root:

- `/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared`

Datasets:

- `arguana`
- `climate-fever`
- `cqadupstack`
- `dbpedia-entity`
- `fever`
- `fiqa`
- `hotpotqa`
- `msmarco`
- `nfcorpus`
- `nq`
- `quora`
- `scidocs`
- `scifact`
- `trec-covid`
- `webis-touche2020`

Important caveat: this is the local shared BEIR15 artifact face. Most datasets
are sampled candidate faces, not the full official BEIR corpora. Qrels were
filtered to positive documents that actually exist in each local document set.
This is therefore the right fast multi-dataset representation check, but not a
full-corpus production posting benchmark.

## Command

```bash
python3 scripts/research_sae_m367_beir15_coordinate_posting_eval.py \
    --active-dims 8,16,32,64,96,128,192,256,384,512,768
```

Output:

- `/tmp/ii42-m367-beir15-coordinate-posting/m367_beir15_coordinate.json`
- `/tmp/ii42-m367-beir15-coordinate-posting/m367_beir15_coordinate.md`

Runtime: 26.29 seconds locally.

## Macro Results

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Dense O@10 | Dense O@100 | Mean touched ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_exact | 0.8533 | 0.8722 | 0.7759 | 0.6947 | 1.0000 | 1.0000 | 1.0000 |
| coordinate_k8 | 0.4605 | 0.4001 | 0.2751 | 0.1980 | 0.1344 | 0.1694 | 0.1325 |
| coordinate_k16 | 0.6029 | 0.5708 | 0.4411 | 0.3477 | 0.2267 | 0.2373 | 0.3918 |
| coordinate_k32 | 0.7258 | 0.7217 | 0.5956 | 0.4968 | 0.3525 | 0.3450 | 0.8289 |
| coordinate_k64 | 0.7991 | 0.8119 | 0.6967 | 0.6046 | 0.5001 | 0.4907 | 0.9987 |
| coordinate_k96 | 0.8253 | 0.8411 | 0.7300 | 0.6449 | 0.5942 | 0.5887 | 1.0000 |
| coordinate_k128 | 0.8366 | 0.8455 | 0.7426 | 0.6580 | 0.6588 | 0.6577 | 1.0000 |
| coordinate_k192 | 0.8457 | 0.8608 | 0.7595 | 0.6776 | 0.7485 | 0.7486 | 1.0000 |
| coordinate_k256 | 0.8501 | 0.8671 | 0.7673 | 0.6860 | 0.8072 | 0.8110 | 1.0000 |
| coordinate_k384 | 0.8523 | 0.8685 | 0.7719 | 0.6921 | 0.8820 | 0.8915 | 1.0000 |
| coordinate_k512 | 0.8518 | 0.8719 | 0.7745 | 0.6944 | 0.9346 | 0.9438 | 1.0000 |
| coordinate_k768 | 0.8533 | 0.8722 | 0.7759 | 0.6947 | 1.0000 | 1.0000 | 1.0000 |

## Per-Dataset k512

| Dataset | Docs | Eval queries | Dense NDCG@10 | k512 NDCG@10 | Dense R@100 | k512 R@100 | k512 O@10 | k512 touched ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| arguana | 2000 | 100 | 0.6650 | 0.6683 | 1.0000 | 1.0000 | 0.9700 | 1.0000 |
| climate-fever | 2000 | 100 | 0.6022 | 0.6021 | 0.9705 | 0.9685 | 0.9450 | 1.0000 |
| cqadupstack | 2000 | 100 | 0.8295 | 0.8206 | 0.9671 | 0.9655 | 0.9350 | 1.0000 |
| dbpedia-entity | 3357 | 100 | 0.7202 | 0.7193 | 0.9091 | 0.9017 | 0.9370 | 1.0000 |
| fever | 2000 | 100 | 0.9988 | 0.9983 | 1.0000 | 1.0000 | 0.9110 | 1.0000 |
| fiqa | 2000 | 100 | 0.7124 | 0.7054 | 0.9211 | 0.9173 | 0.9360 | 1.0000 |
| hotpotqa | 2000 | 100 | 0.9573 | 0.9560 | 1.0000 | 1.0000 | 0.9170 | 1.0000 |
| msmarco | 4102 | 43 | 0.7498 | 0.7432 | 0.8207 | 0.8212 | 0.9558 | 1.0000 |
| nfcorpus | 2063 | 100 | 0.4566 | 0.4552 | 0.3632 | 0.3626 | 0.9180 | 1.0000 |
| nq | 2000 | 100 | 0.9963 | 0.9963 | 1.0000 | 1.0000 | 0.9240 | 1.0000 |
| quora | 2000 | 100 | 0.9918 | 0.9914 | 1.0000 | 1.0000 | 0.9470 | 1.0000 |
| scidocs | 2000 | 100 | 0.4411 | 0.4435 | 0.6920 | 0.6840 | 0.9230 | 1.0000 |
| scifact | 2000 | 100 | 0.8097 | 0.8078 | 0.9800 | 0.9800 | 0.9080 | 1.0000 |
| trec-covid | 17537 | 50 | 0.8587 | 0.8569 | 0.1958 | 0.1949 | 0.9140 | 1.0000 |
| webis-touche2020 | 2000 | 49 | 0.8497 | 0.8523 | 0.9801 | 0.9808 | 0.9776 | 1.0000 |

## Interpretation

The quality side is strong.

At k512, dense-coordinate postings are essentially dense on this BEIR15 face:

- NDCG@10: 0.7745 vs dense 0.7759;
- MAP@100: 0.6944 vs dense 0.6947;
- MRR@20: 0.8719 vs dense 0.8722;
- dense top-10 overlap: 0.9346;
- dense top-100 overlap: 0.9438.

At k384, quality is still very close:

- NDCG@10: 0.7719 vs dense 0.7759;
- MAP@100: 0.6921 vs dense 0.6947;
- dense top-10 overlap: 0.8820.

The efficiency side is the blocker.

Raw coordinate postings lose the useful first-stage pruning region quickly:

- k8 touches 13.25% of docs but quality is far below dense;
- k16 touches 39.18% of docs but quality is still far below dense;
- k32 touches 82.89% of docs and is still materially below dense;
- k64 already touches 99.87% of docs;
- k96 and above touch effectively 100% of docs.

So M367 proves the representation point but not the product-index point:

- signed dense coordinates can preserve dense retrieval very well;
- learned SAE atoms are not currently needed to approach dense quality;
- however, raw coordinate postings create too much candidate fanout at the
  active-k values needed for dense-quality preservation.

## Verdict

This route is valuable, but the success criterion changes.

Dense-coordinate postings should become the dense-only quality baseline to beat.
Any learned SAE, rotated-coordinate, quantized, or block-pruned atom system must
match k384/k512 quality while reducing fanout far below raw coordinate postings.

The next useful experiment is not another BM25 admission rescue. It is a
dense-only efficiency experiment:

1. Try rotated / orthogonal dense coordinates.
2. Measure whether k32-k96 can retain more dense quality at lower touched ratio.
3. Add block-max or impact pruning on coordinate postings.
4. Only return to learned atoms if they beat this quality-fanout frontier.
