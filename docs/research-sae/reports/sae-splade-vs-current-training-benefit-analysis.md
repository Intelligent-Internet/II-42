# SAE-SPLADE Vs Current Text-Atom Training Benefit Analysis

Date: 2026-05-15

## Scope

This note compares the direction from
[From Tokens to Concepts: Leveraging SAE for SPLADE](https://arxiv.org/abs/2604.21511)
with the current ii42 SAE training line. The goal is not to decide
whether ordinary SPLADE should be added to SQL. That was already tested as an
offline baseline and did not beat the current BM25+SAE path. The question here
is narrower and more useful:

```text
Can an SAE-SPLADE-style concept vocabulary improve our direct text -> sparse
atoms path, and does it help remove the remaining dense-teacher dependency?
```

## Current Training Baseline

The current best direct text-to-atoms result is the full15 candidate-budget
student:

```text
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-token-char/
```

It uses:

- Snowflake/snowflake-arctic-embed-m-v2.0 + SAE as the teacher;
- a lightweight token/token-char text encoder as the student;
- separate query/document heads over the same teacher atom namespace;
- retrieval-aware losses, dense-teacher listwise signals, coverage losses,
  fanout loss, and candidate-budget loss;
- the unified BM25+SAE sparse-impact evaluation path.

Mean full15 quality:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| `bm25_teacher_sae` | 0.8425 | 0.8458 | 0.7507 | 0.7249 |
| `bm25_student_atoms_w0p5` | 0.8280 | 0.8292 | 0.7223 | 0.6931 |
| `bm25_student_atoms` | 0.8324 | 0.8291 | 0.7225 | 0.6913 |
| `bm25_student_atoms_w1` | 0.8340 | 0.8240 | 0.7142 | 0.6782 |

The current engineering path is also already native-index shaped. The full15
C payload smoke reaches exact parity with the Python evaluator:

| Exact Match Rate | Recall@100 | NDCG@10 | MAP@100 | Candidates | SAE Posts | BM25 Posts | Payload MB |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1.0000 | 0.8286 | 0.7250 | 0.6880 | 738.5 | 787.4 | 381.4 | 6.64 |

The important gap is therefore not physical-index feasibility. The gap is model
generation:

```text
current student < Snowflake-SAE teacher
```

The student is useful enough to keep, but it has not proven that dense
embedding generation can be removed without quality loss.

## Existing SPLADE Baseline

We already tested a practical public SPLADE checkpoint:

```text
model: naver/splade_v2_distil
datasets: scifact, scidocs, nfcorpus, arguana, fiqa
doc_active_dims: 128
query_active_dims: 64
```

Five-dataset mean:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| `bm25` | 0.7035 | 0.5904 | 0.5031 | 0.4134 | 5.5364 |
| `bm25_sae` | 0.7934 | 0.6742 | 0.5952 | 0.4987 | 5.6762 |
| `splade` | 0.7519 | 0.6324 | 0.5559 | 0.4615 | 6.3750 |
| `bm25_splade` | 0.7431 | 0.6399 | 0.5539 | 0.4572 | 7.9785 |
| `bm25_sae_splade` | 0.7899 | 0.6814 | 0.6032 | 0.5043 | 7.6224 |

This says two things:

- ordinary off-the-shelf SPLADE is not a better immediate next index source;
- SPLADE remains useful as a control because it proves a learned sparse text
  encoder can fit the same inverted sparse-impact abstraction.

The SAE-SPLADE paper is not interesting to us because it says "use SPLADE".
It is interesting because it replaces the output vocabulary with SAE concept
latents and trains the retrieval model around that concept vocabulary.

## What SAE-SPLADE Changes

The paper and its
[reference repository](https://github.com/yzong12138/sae_splade) describe a
different representation path:

```text
PLM token hidden states
-> TopK SAE concept encoder
-> SPLADE-style document-level max aggregation
-> retrieval fine-tuning
-> inverted index over learned concept ids
```

The key differences from our current student are:

| Area | Current text atom student | SAE-SPLADE-style concept path |
| --- | --- | --- |
| Input representation | token / char features with shallow learned encoder | contextual token states from a PLM |
| Atom teacher | Snowflake dense embedding -> SAE | SAE over PLM token states |
| Pooling | text-level prediction into teacher atom ids | SPLADE-style max pooling over token concept activations |
| Cost control | candidate-budget, fanout, index-side caps | TopK SAE, FLOPs/QD-FLOPs, query/doc activation budgets |
| Serving dependency | can serve without Snowflake only if student quality is accepted | can serve as a standalone open-source sparse encoder |
| Main risk | student only partially mimics teacher support | online query encoder may be heavier than current shallow student |

## Expected Benefit

### 1. Better Atom Semantics Than The Current Student

Our student is a compressed imitation of a sentence-level dense teacher. It has
no access to token-level contextual states. SAE-SPLADE gives the concept
vocabulary a cleaner source: token hidden states before sparse aggregation.

That can help with:

- query/document asymmetry;
- phrase-local evidence;
- synonymy/polysemy splitting;
- high-level concept atoms that are not tied to literal vocabulary ids;
- multilingual or cross-domain overlap diagnostics.

This is the strongest reason to try it.

### 2. Cleaner Path To Removing Dense Embeddings

The current teacher path still starts from Snowflake dense embeddings. Even if
the student serves directly from text, the best atom labels are inherited from a
dense embedding space.

SAE-SPLADE gives a more native route:

```text
text -> open-source token encoder -> SAE concept atoms -> unified sparse index
```

If it matches the current student and approaches the Snowflake-SAE teacher, it
is a stronger product story than "we distilled a dense embedding model into
sparse atoms".

### 3. Better Cost Metrics For Model Training

The paper's QD-FLOPs framing matches our index bottleneck better than pure
activation count. Our current candidate-budget loss is the right instinct, but
the reports should expose both abstract and physical sparse costs:

```text
query_active_atoms
doc_active_atoms
estimated_qd_flops
sum_query_atom_df
candidate_docs
postings_read
rerank_doc_terms
```

This is a direct benefit even before we implement a new encoder.

### 4. Larger Concept Namespace Becomes A Controlled Experiment

The current teacher uses 8192 latents. SAE-SPLADE studies larger latent
vocabularies, including `2^16`. A larger namespace may reduce overloaded atoms,
but only if actual posting reads stay bounded. This gives us a concrete
experiment instead of arbitrary atom-count tuning.

## Risks

### 1. It May Not Beat The Current Teacher

The current Snowflake-SAE teacher is strong. A DistilBERT-sized SAE-SPLADE
control may be more efficient but lower quality. That is still useful only if
the sparse-cost win is large enough.

### 2. Online Query Encoding May Be More Expensive

Our current shallow student is much cheaper than a PLM query encoder.
SAE-SPLADE must win at index traversal or recall enough to justify that online
cost. Otherwise it becomes another heavy neural front-end.

### 3. Existing SPLADE Baseline Warns Against Shortcut Conclusions

The off-the-shelf SPLADE run did not beat BM25+SAE. A paper-aligned control
must be evaluated as a new model family, not assumed superior because SPLADE is
usually strong.

### 4. Larger Latent Vocabularies Can Increase Fanout

A wider concept vocabulary can improve semantic granularity, but it can also
increase index size and doc-vector storage. Promotion must require physical
cost wins, not just recall gains.

## Recommendation

There is enough expected benefit to run a controlled SAE-SPLADE-style
experiment, but not enough evidence to replace the current training line.

The correct order is:

1. Add QD-FLOPs-style metrics to current reports first.
2. Build a small paper-aligned control on the five-dataset subset.
3. Compare against `bm25`, `bm25_sae`, current text-student atoms, and the
   existing SPLADE baseline.
4. Only scale to full15 and larger latent vocabularies if the five-dataset
   control beats ordinary SPLADE and approaches the current student.
5. Promote it toward SQL/native only if it either closes the quality gap to the
   Snowflake-SAE teacher or materially reduces physical sparse cost.

Initial gates:

| Gate | Requirement |
| --- | --- |
| Baseline gate | Beat pure BM25 and `naver/splade_v2_distil` on the five-dataset matrix. |
| Student gate | Reach within 0.01 Recall@100 and 0.02 MRR@20 of the current text-student on the same subset, or show more than 30% lower sparse cost. |
| Teacher gate | On full15, close at least half of the gap between current text-student and Snowflake-SAE teacher. |
| Cost gate | Report `candidate_docs`, `postings_read`, and `estimated_qd_flops`; do not accept quality gains from unbounded fanout. |
| Serving gate | Include online query encoder latency separately from index traversal latency. |

## Bottom Line

SAE-SPLADE is not an immediate replacement for our current training. Its value
is that it points to a better training shape for the next generation of the
unified sparse engine:

```text
contextual text encoder
-> learned SAE concept vocabulary
-> cost-aware sparse retrieval fine-tuning
-> one BM25+concept sparse-impact payload
```

The expected payoff is medium-to-high if it can preserve the current
BM25+SAE quality while removing dense embeddings from the serving path. The
near-term action is a controlled concept-encoder experiment with strict
quality/cost gates, not broad product integration.
