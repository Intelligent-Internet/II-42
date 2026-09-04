# M1917-M1920 Parent And PPLX Closure

Date: 2026-07-13

Decision: **use frozen OpenSearch sparse-v2 as the next product parent, retain
M1914 as the compact research control and P1 as the dense-faithfulness control.
Reject the M1918 continuation checkpoint, reject threshold salvage, and do not
authorize PPLX LoRA as a sparse parent.**

This stage produced a parent-selection result and two useful stop results. It
did not produce a new trained checkpoint that beats the frozen OpenSearch
baseline on the common native surface.

## 1. Frozen Parent Matrix

M1917 evaluated all parents on the same full official FiQA, ArguAna, NFCorpus,
and SciFact corpora, using the same exact PostgreSQL normalized-postings path,
candidate depth 1,000, query IDs, qrels, and PPLX dense reference.

| Route | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB | Storage MiB | p95 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| P1 | 0.444319 | 0.352600 | **0.731116** | 0.478968 | **0.493404** | **0.866933** | **703.59** | 108.038 |
| M1914 | 0.452702 | 0.353850 | 0.725207 | 0.490606 | 0.401635 | 0.860291 | 1026.16 | **90.848** |
| OpenSearch | **0.455414** | **0.355934** | 0.716066 | **0.495727** | 0.404142 | 0.854005 | 1276.27 | 176.284 |

These storage and latency values compare equal PostgreSQL implementations;
they are not measurements of OpenSearch's optimized Lucene engine.

The routes answer different questions:

- OpenSearch is the best frozen ranking/product parent. It has the strongest
  macro NDCG, MAP, and MRR and a mature heterogeneous-distillation recipe.
- M1914 remains the compact and efficient learned-sparse control. It has the
  best common-backend latency and better Recall than OpenSearch, but weaker
  macro head quality and a severe FiQA gap.
- P1 remains the dense-faithfulness and Recall control. It has the strongest
  O@100, CUB, and Recall, but weak NFCorpus and SciFact ranking.

OpenSearch is selected over M1914 for deeper work because it wins the frozen
macro head metrics and was the only parent that showed a measurable heldout
training response. This does not mean M1914 is inferior for every product
constraint; it remains the better compactness/latency control.

## 2. Teacher And Trainability Evidence

The 170,249-row public RLHN surface uses NQ, HotpotQA, and FEVER. The split has
153,251 training rows and 16,998 heldout rows with zero normalized-query hash
overlap. BEIR evaluation rows were not used to select the checkpoint.

On the scaled heldout audit, PPLX was the strongest individual teacher:

| Teacher | Pairwise accuracy | Positive top1 |
| --- | ---: | ---: |
| M1914 | 0.905761 | 0.796875 |
| OpenSearch | 0.900315 | 0.800781 |
| PPLX | **0.928928** | **0.865234** |

PPLX and OpenSearch are complementary rather than interchangeable. On the
M1917 native surface, 6.9265% of relevant top100 pairs were PPLX-only and
6.4385% were OpenSearch-only.

The M1914 canary did not pass its parent-response gate. OpenSearch did. Its
selected step 1,024 improved heldout pairwise accuracy by 0.006335, positive
top1 by 0.011719, and PPLX KL by 3.6649%, while retaining parent support at
0.995215 and changing sample nnz by only 1.000607x. Step 2,048 was already
worse, so the local response did not justify simply training longer.

## 3. M1918 Exact Native Closure

The selected OpenSearch checkpoint was regenerated over every document and
query and published into the same native PostgreSQL path. The evaluation used
the same 32-query warmup as M1917.

| Surface | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Frozen OpenSearch | **0.455414** | **0.355934** | **0.716066** | **0.495727** | **0.404142** | 0.854005 |
| M1918 checkpoint | 0.453018 | 0.354181 | 0.713600 | 0.492537 | 0.402788 | **0.855070** |
| Delta | -0.002396 | -0.001754 | -0.002467 | -0.003190 | -0.001354 | +0.001066 |

The checkpoint slightly expanded candidate upper bound, but every primary
macro metric regressed and no dataset delivered a complete quality win.
Storage increased to 1.0304x. Full-corpus document nnz increased to 1.0246x
and maxDF moved from 0.487682 to 0.511191. This is a real transfer failure:
candidate-set heldout improvements did not preserve full-corpus score geometry.

The initial cold latency measurement was discarded because M1917 used a
warmup. Under the corrected protocol, M1918 p95 was 111.613 ms versus the
baseline's 176.284 ms. Latency therefore passed; quality did not.

## 4. M1920 Cost-Shaping Diagnostic

A single global, qrels-free impact floor of 0.05 was the first threshold that
restored mean nnz and maxDF to at most the frozen OpenSearch ranges across all
four document and query surfaces. It was tested once as a diagnostic, not as
a threshold search.

| Surface | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB | Storage ratio | p95 ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M1920 t=0.05 | 0.453059 | 0.354450 | 0.713403 | 0.492801 | 0.402583 | 0.854919 | 0.9234x | 0.8837x |
| Delta vs frozen | -0.002355 | -0.001484 | -0.002663 | -0.002926 | -0.001559 | +0.000915 | - | - |

Thresholding repaired cost and recovered a small part of raw M1918's head
loss, but did not restore frozen quality. FiQA Recall became worse than the raw
checkpoint, and only NFCorpus Recall rose by 0.000046 over frozen. This rejects
the hypothesis that low-impact postings were the sole cause. Further threshold
grids would be post-hoc tuning without a new mechanism and are stopped.

## 5. PPLX As Parent Versus Teacher

M1919 tested a no-training tied-vocabulary diagnostic. It projected the PPLX
retrieval state through the model's 151,936-token input embedding matrix and
constructed sparse query/document postings. This is a shape test, not an LM
head and not a LoRA experiment.

At the expanded 512-row gate, the best Q128/D512 surface achieved:

| Metric | Result |
| --- | ---: |
| Dense score correlation | 0.797279 |
| Dense top1 agreement | 0.708984 |
| Label pairwise accuracy | 0.855332 |
| Positive top1 | 0.681641 |
| Parent top1 agreement | 0.654297 |
| Document maxDF | 0.181269 |
| Query maxDF | 0.107422 |

Root parity passed, but posting shape failed the 0.70 positive-top1 floor even
at 512 document and 128 query postings. Increasing support improved dense
correlation but did not establish a competitive sparse retrieval surface.

PPLX remains useful as a heterogeneous semantic teacher because it has the
strongest heldout ranking and unique positive coverage. It is not authorized
as the next sparse parent. A PPLX LoRA would be trying to repair an unproven
output geometry while changing a much larger model; the present data cannot
separate model capacity from representation mismatch.

## 6. Relation To Mature Literature

The scale difference is material. The OpenSearch sparse-v2 recipe used about
5.36 million pretraining queries for 150,000 steps, then 502,939 MS MARCO
queries for 50,000 fine-tuning steps. It also combines dense and sparse
teachers with an IDF-aware objective. M1918 was a response test, not a faithful
reproduction of that training program.

Mistral-SPLADE and LACONIC likewise use staged, large-data distillation rather
than a tiny output-head adjustment. DF-FLOPS directly targets corpus document
frequency because batch FLOPS alone does not prevent a few high-DF terms from
dominating traversal. These results support a full-scale mature-recipe
reproduction; they do not support extending the same 2,048-step local M1918
run or launching PPLX LoRA from the rejected tied-vocabulary shape.

References:

- OpenSearch sparse-v2: https://arxiv.org/html/2411.04403
- Mistral-SPLADE: https://arxiv.org/html/2408.11119
- LACONIC: https://arxiv.org/html/2601.01684
- DF-FLOPS: https://arxiv.org/html/2505.15070

## 7. Final Route Decision

The best deployable model from this stage is still frozen OpenSearch sparse-v2.
No new trained route surpassed it. The stage nevertheless resolves the parent
question:

1. Use OpenSearch as the main learned-sparse product parent.
2. Keep M1914 as the compact/latency and architecture-control branch.
3. Keep P1 as the dense-faithfulness/Recall control.
4. Use PPLX only as one teacher inside a heterogeneous target.
5. Do not continue M1918, threshold grids, or PPLX tied-vocabulary LoRA.

The next justified training program is a faithful OpenSearch-scale continuation
with millions of public rows, staged teacher construction, periodic corpus-DF
refresh, and native validation during training. It must be budgeted as a model
reproduction, not described as another canary. Promotion still requires a
fresh shared15/native gate; lower training loss is not sufficient.

## Artifacts

- `ii42-m1917-tri-parent-native-matrix.json`
- `ii42-m1917-parent-complementarity.json`
- `ii42-m1918-parent-response-opensearch-balanced2048.json`
- `ii42-m1918-opensearch-checkpoint-native.json`
- `docs/research-sae/reports/m1900-m1999/ii42-m1918-opensearch-checkpoint-native-report.md`
- `ii42-m1919-pplx-tied-vocabulary-posting.json`
- `ii42-m1919-pplx-tied-vocabulary-pareto512.json`
- `ii42-m1920-thresholded-checkpoint-native.json`
- `docs/research-sae/reports/m1900-m1999/ii42-m1920-thresholded-checkpoint-native-report.md`
