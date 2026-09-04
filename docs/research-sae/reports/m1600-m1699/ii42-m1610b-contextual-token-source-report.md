# M1610B Contextual Token Source Report

## Executive Decision

**Stop M1610B.** Contextual token routing produces a high-capacity document
posting source after increasing the namespace to 8,192 keys, but neither the
direct query router, a 32x larger query action budget, nor qrels-free corpus
co-occurrence can select the tail keys needed for dense equivalence.

### Fixed-Depth Metric Correction

M1620 later found that the shared `overlap_indices` helper normalized an
underfilled result by the number of returned candidates rather than the fixed
dense target depth. The original M1610B tables therefore overstate O@100 and
O@256 whenever fewer than K candidates were returned. With a strict fixed-K
denominator, the 8,192-key direct route at `0.15x` is O@100 `0.236960` and
O@256 `0.130109`, not `0.510025` and `0.506198`.

The correction strengthens the no-go decision: the conditional source oracle
remains near O@100 `1.0` and O@256 `0.88`, while direct observability is much
weaker than initially reported. Historical values below are retained to make
the original run reproducible, but must be read as underfill-normalized values.

The result isolates a query-time observability failure, not a lack of source
capacity or training depth. No trained checkpoint, second seed, or native
BEIR evaluation is authorized.

## Tested Hypothesis

M1610B adapted the CITADEL/CoRGII routing mechanisms to the strict product
surface:

```text
frozen BGE contextual token states
    -> shared latent-key router
    -> hard query/document Top-K keys and impacts
    -> one scalar posting namespace
    -> exact fixed-budget posting union
```

The loss directly trained query/document key agreement with in-batch
contrastive competition, a positive-negative margin, load balance, activation
control, and a geometry anchor. It did not use qrels or dataset identity.
The canary used 1,000/250 disjoint query rows and 8,988/2,250 documents.

## S1: 2,048-Key Capacity Gate

The initial router was better than a matched static lexical-token control,
but its document postings were too broad and its source oracle missed the
dense tail.

| Surface at 0.15x cap | O@10 | O@100 | O@256 | Reads | Max DF |
| --- | ---: | ---: | ---: | ---: | ---: |
| lexical token control | 0.815144 | 0.361697 | 0.314304 | 0.079746x | 0.722667 |
| initial contextual router | 0.854400 | 0.503662 | 0.481304 | 0.050396x | 0.129778 |
| final trained router | 0.853956 | 0.494218 | 0.454086 | 0.065998x | 0.068889 |
| source oracle | 0.984400 | 0.967600 | 0.676669 | 0.149568x | 0.129778 |

Training flattened max DF, but it reduced hard O@100/O@256. The best eligible
checkpoint remained initialization. Failure anatomy supported one structural
repair: increase key resolution by 4x rather than tune loss weights.

## S2: 8,192-Key Source Repair

The locked 8,192-key initialization used 4,493 active keys. Documents emitted
8.24 unique keys on average, max DF fell to `0.056889`, and the source oracle
became strong:

| Surface at 0.15x cap | O@10 | O@100 | O@256 | Reads | Query keys |
| --- | ---: | ---: | ---: | ---: | ---: |
| direct qTopK=2 | 0.772854 | 0.510025 | 0.506198 | 0.026206x | 4.55 |
| source oracle | 1.000000 | 1.000000 | 0.900384 | 0.149307x | 167.18 |

This is a real capacity result, although max DF still narrowly misses the
strict `0.05` target. The gap is too large to interpret as an impact-scoring
problem: the direct query route does not admit the relevant tail documents at
all.

## Query Action-Budget Audit

The document source and router weights were frozen. Only query Top-K was
increased from 2 to 64.

| qTopK | O@10 | O@100 | O@256 | Reads | Query keys |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 0.772854 | 0.510025 | 0.506198 | 0.026206x | 4.55 |
| 4 | 0.840756 | 0.487431 | 0.477357 | 0.043989x | 8.48 |
| 8 | 0.900400 | 0.498650 | 0.467073 | 0.070340x | 15.68 |
| 16 | 0.938000 | 0.543185 | 0.468887 | 0.108103x | 29.80 |
| 32 | 0.952800 | 0.582523 | 0.467709 | 0.138834x | 57.26 |
| 64 | 0.954400 | 0.597146 | 0.466802 | 0.149337x | 109.78 |

All rows use the `0.15x` cap. More keys recover dense head membership, but
O@256 decreases from `0.506198` to `0.466802`. Local token cosine therefore
orders head-like routes, not the document keys that cover the dense tail.

## Corpus Co-occurrence Audit

CoRGII motivates static co-occurrence multiprobing. A final finite diagnostic
built key-key co-occurrence only from the indexed validation documents. It
expanded the locked qTopK=2 query keys without labels or dense scores.

| Neighbours/key | O@10 | O@100 | O@256 | Reads | Query keys |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 4 | 0.813144 | 0.490944 | 0.480274 | 0.056032x | 16.57 |
| 8 | 0.849200 | 0.471753 | 0.452224 | 0.077193x | 28.47 |
| 16 | 0.885600 | 0.462723 | 0.419056 | 0.112368x | 50.58 |
| 32 | 0.898800 | 0.474691 | 0.405490 | 0.139008x | 85.86 |

Again, all rows use the `0.15x` cap. Co-occurrence increases head overlap as
it adds keys, but progressively damages tail overlap. It does not recover the
oracle action set.

## Interpretation

1. Contextual tokenization is better than literal lexical routing, but that
   alone is not a dense-equivalent posting source.
2. Increasing codebook resolution fixes much of source capacity and DF, but
   it makes the required query action set much larger and less locally
   observable.
3. The direct router, broad local probing, and static corpus co-occurrence all
   show the same head/tail inversion. This is stronger evidence than one
   failed training loss.
4. An impact head or longer training cannot rank documents that the posting
   union never admits. Training the 8,192-key router is therefore not
   authorized after the source-policy gate fails.
5. Full CITADEL retains token-vector interactions after routing. That residual
   multi-vector score is useful in its own product, but it is outside the
   tested scalar unified-posting contract and cannot be claimed as a repair.

## Reproducibility

- Host: `spark-1`.
- Container: `nvcr.io/nvidia/pytorch:26.03-py3`.
- ClearML integration task: `0d522e150cd4455485968775849fc58b`.
- ClearML 2,048-key task: `15f6054b88344e0ab5607fc7e9afce11`.
- ClearML 8,192-key capacity tasks:
  `02c549a1fdf849a9baea1622557a38c0` and
  `fe74830706f242d3a26bb46545b0e78c`.
- Root:
  `/home/huoju/leask/runs/ii42-m1610-retrieval-source-v1`.
- Locked 8,192-key checkpoint SHA-256:
  `9bba79d1e93870152bd25d3d7d861292ce98dc6f9865a76d460cfff87f92fa55`.
- Query-budget summary SHA-256:
  `c9594d149322c8a992cd05a0322df3cbee5c6b4fa0386820cbc4395037a5cc3f`.
- Co-occurrence summary SHA-256:
  `98279fed7cd3824604cb00cd8cfb641b85e1bb705a9042b3398e77e2a2a57a53`.
