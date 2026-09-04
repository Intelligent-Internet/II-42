# M1900 Learned-Sparse Reset Research Map

Date: 2026-07-12

II-Commons arXiv cutoff: `2026-07-10`

## Reset Principle

The project will no longer start from a project-specific posting loss. It will
first reproduce a published learned-sparse system as a complete chain:

```text
training provenance -> frozen checkpoint -> complete corpus encoding
-> one physical inverted index -> exact quality and measured cost
```

Only after a route passes that chain may the project change its representation
or training objective. A lower training loss, a sampled corpus, or a raw union
touch estimate is not a milestone.

## Route Matrix

| Route | Public checkpoint | Code/license status | Existing local evidence | Role |
| --- | --- | --- | --- | --- |
| OpenSearch sparse v2 | yes | Apache-2.0 | M1640/M1660 full FiQA + exact BMP pass | frozen product baseline |
| IBM Granite 30M Sparse | yes, pinned revision `ad82b1fd...` | Apache-2.0 checkpoint; mixed public/internal training data | M1913 full FiQA + exact BMP pass; compact Recall/cost positive, head metrics lower | next dense-root optimization parent |
| Unified LSR framework | tag `v1.0.0`, commit `71cd1d31...` | Apache-2.0 | exact paper configs for SPLADE/uniCOIL/EPIC and direct Anserini impacts | auditable optimization framework |
| SPLADE-v3 DistilBERT | cached revision `2db06b86...` | CC-BY-NC-SA-4.0 | M1640 shared3; NFCorpus row gate failed | research quality baseline |
| SAE-SPLADE | no | code pinned; no repo license | M1902/M1903 ranking-positive, high-DF | small-backbone mechanism line |
| Standard SPLADE paired control | generic DistilBERT warm start only | local paired implementation; official Naver reference is CC-BY-NC-SA-4.0 | M1905 is the only post-ramp survivor and passes disjoint confirmation | bounded mechanism control |
| SPLARE | no | paper only; component SAEs public | M1912 artifacts pass; M1912A frozen M1510 exact BMP cost passes but quality remains weak | pretrained-SAE architecture target |
| Latent Terms | no | paper only | M1911 five-seed near OpenSearch quality + exact BMP pass; native cost high | secondary qrels-free latent milestone |
| CL-SR | no release checkpoint | Apache-2.0 code | published in-domain MS MARCO latent index | structural control |
| Embedding Scope | extracted features only | MIT code | retrieval-oriented dense-embedding SAE | faithfulness-loss control |
| Li-LSR | no public checkpoint found | paper recipe; no linked code release | modern engines justify relaxed document expansion | inference-free query control |
| Wacky Weights reproduction | code linked by paper | paper CC-BY-4.0 | larger vocabularies expose more ranking shortcuts | expansion-mechanism audit |
| OpenSearch doc v3 GTE | yes | Apache-2.0 | not locally reproduced | product upper control with seen BEIR rows |
| LACONIC | yes | research artifact | prior scaling reference only | large-model sparse scaling reference |

OpenSearch doc v3 GTE was trained on FiQA, NFCorpus, SciFact, FEVER, and
HotpotQA among other sources. Those rows cannot be used as clean
generalization evidence. SPLADE-v3 is non-commercial and remains a research
control even when its quality is stronger.

## What The Literature Changes

### SAE-SPLADE

SAE-SPLADE uses a separately reconstructed width-65,536 TopK-8 SAE followed by
240,000 retrieval steps with KL, margin MSE, and a 6,000-step FLOPS ramp. Its
reported TopK-8 result is MS MARCO MRR@10 `0.376`, BEIR mean NDCG@10 `0.492`,
QD FLOPS `0.67`, and mean document length `109`.

M1903 reached only 10.6M token exposures and 16,000 query presentations,
roughly one-thousandth to one-five-hundredth of the paper exposure. More
importantly, it stopped before the FLOPS ramp completed. M1904 is a bounded
schedule diagnostic. Its maxDF is measured over the disjoint candidate
documents, not the complete MS MARCO corpus, so it cannot by itself establish
native index cost. M1905 repeats the full ramp with standard vocabulary SPLADE
on the identical rows and teacher. It selects an eligible 8,000-step
checkpoint, passes disjoint confirmation, and dominates the SAE checkpoint in
ranking, KL, nnz, FLOPS, and sampled maxDF. This localizes the bounded failure
to the from-scratch SAE basis rather than the common schedule. It does not turn
the bounded standard checkpoint into an official or native milestone.

### SPLARE

SPLARE freezes a large pretrained SAE, inserts it at an intermediate LLM
layer, and trains only LoRA adapters in the backbone with cross-encoder KL and
FLOPS. It reports that:

- intermediate layers around two-thirds depth work best;
- sequence-level sparsity remains high despite token-level SAE sparsity;
- temperature must be calibrated to SAE logit scale;
- inference TopK `(40,400)` is robust and reaches about 5 ms/query on 8.8M
  passages with Seismic;
- training from a random output head is substantially worse.

This is the strongest evidence against another from-scratch project SAE. If
M1904 fails, the next SAE line should begin from a public pretrained SAE and
LoRA, not more reconstruction loss variants.

M1912A also revises the old M1510 engine interpretation. After an exact
namespace-only compaction from 65,536 declared dimensions to 34,510 used
dimensions, the frozen surface retains every posting and dot product. Patched
BMP preserves strict top-100 boundaries, retains 99.919% of float Recall, and
runs at 15.22 ms p95 versus 29.80 ms exhaustive. Raw union touch is therefore
not a valid reason to reject this architecture. M1510 still fails as a model
parent because its FiQA quality remains below OpenSearch sparse-v2.

### Latent Terms

Latent Terms freezes a retrieval-trained dense backbone, trains a standard
TopK SAE on qrels-free FineWeb-Edu token states, sum-pools latent activations,
applies a square-root transform, and scores with BM25. The paper reports Nomic
Latent Terms BEIR15 mean NDCG around `0.526`, compared with Nomic dense around
`0.521` and SPLADE-v3 around `0.502` on its stated matrix.

M1910 proves this score is compatible with the exact BMP product form. The
paper's stated `30B unique tokens` and under-two-hour A100 training claim are
not mutually plausible without additional implementation detail, so a local
reproduction must report actual token exposure rather than claim exact scale.

Its high-frequency behavior is not equivalent to M1904. SAE-SPLADE scores SAE
features by an unnormalized dot product, so a universal latent remains an
unpenalized shared score and an expensive posting. Latent Terms deliberately
uses corpus DF and BM25 IDF over a heavy, approximately Zipfian feature head.
The paper reports saturated terms as part of the mechanism, and M1910 shows
that pruning the highest-DF 1% hurts full FiQA Recall. M1904's maxDF stop must
therefore not be copied into M1911 as a pre-training pruning rule.

The clean M1911 reproduction is complete. It pins Nomic weights and the
separate Nomic custom-code revision, follows the model-card task prefixes,
trains five TopK-16 seeds, and keeps seed selection qrels-free. Its five-seed
FiQA mean reaches NDCG `0.367006`, MAP `0.309725`, Recall `0.655292`, and MRR
`0.448176`. The qrels-free selected seed passes exact BMP with Recall
`0.658295`, a 327.7 MB index, and 27.50 ms p95. This is a real representation
milestone but remains much denser than the mature vocabulary-sparse controls.
Removing the highest-DF 1% latents materially harms quality, so post-hoc
pruning is closed. Unique corpus tokens and repeated training-token
presentations are reported separately.

### Dense-Embedding SAE Controls

CL-SR trains an SAE over final SimLM embeddings on the in-domain 8.8M MS MARCO
corpus, then uses IDF-weighted latent impacts in an inverted index. Its reported
MRR@10 is `0.343` for the efficient configuration and `0.368` for the larger
configuration, versus about `0.412` for the SimLM dense parent. It is a real
single-index success, but not a dense-equivalent or clean zero-shot route.

Embedding Scope adds a retrieval-oriented contrastive recovery objective to an
SAE over dense embeddings. It is useful evidence that reconstruction alone is
not sufficient for final-vector faithfulness. It remains a control because it
does not expose the token-level, BM25-ready vocabulary demonstrated by Latent
Terms or the standalone sparse text encoder demonstrated by SAE-SPLADE.

### Granite 30M Sparse

IBM's released Granite 30M sparse checkpoint is the closest mature control to
the project's original dense-root idea. It starts from a six-layer retriever
trained with retrieval-oriented pretraining, then trains the MLM sparse output
through contrastive knowledge distillation from a larger dense retriever. The
paper reports that FLOPS alone did not make this starting point sufficiently
sparse, so it adds query/document total-NORM regularization. The model card
publishes fixed query/document active-dimension limits of `50/192` and reports
BEIR-13 NDCG `50.8`, comparable to SPLADE-v3 DistilBERT at roughly half its
parameter count.

This route is executable and commercially licensed, but its original training
is not fully reproducible because the data mixture includes IBM-internal and
generated sources. M1913 therefore treats the pinned checkpoint as a frozen
product control. A direct local implementation of its ReLU/log1p/max-pooling
and top-k path matches the official Sentence Transformers encoder exactly on
an untuned two-text audit: identical support, nnz, and impacts with maximum
absolute error `0.0`.

M1913 completes the native gate. Float Recall is `0.666484`; exact BMP retains
`99.886%` and uses 156.3 MB at 12.55 ms p95. This is smaller and faster than
OpenSearch sparse-v2 while improving Recall. NDCG and MRR remain lower, and
MAP misses the locked 95% floor by `0.000583`, so Granite is not the product
default. It is nevertheless the next model parent because it localizes the
remaining problem to head ordering while already satisfying the compact
dense-root representation and native-cost requirements.

M1914 then identifies a transferable output-shape correction without changing
support: query power `1.851864`, document power `0.562796`, and query score
scale `0.696368`. It improves all FiQA BMP primary metrics, keeps index bytes
identical, lowers p95 to 11.55 ms, and transfers macro-positive across ArguAna,
NFCorpus, and SciFact. Post-TopK per-term calibration overfits. M1915 doubles
query/document candidate support on query-disjoint MS MARCO and finds no
ranking signal, so local support reallocation is closed. The current research
parent is M1914, not an expanded-support or per-term variant.

### Efficiency And Calibration

- The unified LSR reproduction framework isolates architecture from training
  recipe. Its controlled experiments find that document term weighting is the
  largest effectiveness contributor, query weighting is modestly useful, and
  query/document expansion partly cancel. With document MLM expansion held
  fixed, replacing query MLM expansion with a weighted non-expanding query
  encoder preserved MS MARCO effectiveness while reducing reported retrieval
  latency by more than 74%. If M1905 survives the full ramp, this asymmetric
  query/document ablation is the first justified product probe; another
  coupled query-and-document loss search is not. The Apache-2.0 `v1.0.0`
  reproduction config is
  `splade_asm_qmlp_msmarco_distil_flops_0.0_0.08`: a non-expanding
  `TransformerMLPSparseEncoder` query path, an expanding
  `TransformerMLMSparseEncoder` document path, Margin-MSE distillation, query
  regularizer `0`, and document FLOPS weight `0.08`.
- The same pinned framework defaults to 150,000 training steps, per-device
  batch 128, and 6,000 warmup steps. M1905 exposes only 80,000 query
  presentations (`10,000 x 8`), so it is an architecture/schedule mechanism
  comparison rather than evidence that a bounded checkpoint has reached the
  mature SPLADE quality ceiling.
- DF-FLOPS shows that ordinary FLOPS can miss high document-frequency terms.
- Seismic and BMP show that raw posting-union touch is not native latency.
- Li-LSR shows that modern sparse engines can make a less-regularized document
  encoder practical, while moving the bottleneck to query encoding. Its Big
  models use roughly 300 document terms and still improve BEIR quality. This
  agrees with M1910: native BMP cost, not a sampled FLOPS proxy, is the product
  gate.
- Two-Step SPLADE shows that a fixed two-stage sparse approximation can improve
  latency, but it is not needed before the one-index baseline is measured.
- Rescaling MLM-Head shows that unnormalized sparse dot-product training is
  sensitive to output projection scale; modern backbones can collapse unless
  initialized in the correct scale range.
- Corpus-specific vocabularies can improve both quality and latency by
  distributing activations over a larger, corpus-relevant namespace.
- The expanded-vocabulary study finds that a pretrained 100K output basis is
  better than a random 100K basis at similar retrieval cost. Vocabulary size
  alone is not capacity: initialization defines the query/document interaction
  surface, and static pruning is applied only after that surface is trained.
- The Wacky Weights reproduction finds that larger vocabularies provide more
  degrees of freedom for semantically unrelated ranking codes, while stricter
  per-input sparsity reduces them. Those codes mainly help the training domain
  and contribute little on BEIR. A random latent head can therefore improve a
  local teacher fit without becoming a reusable semantic vocabulary.

These results refine the M1904 interpretation. A full FLOPS ramp can be a
useful mechanism diagnostic, but low candidate-sample maxDF is not itself a
promotion target. If ranking falls while the proxy improves, the conclusion is
that this schedule trades away useful expansion on this surface. Product cost
must still be measured through the complete BMP index. Conversely, a high-cost
sample proxy cannot rescue a branch whose complete native quality fails.

## Parent Selection And Execution Order

1. M1904 and M1905 close the paired mechanism question: only standard SPLADE
   has a post-ramp survivor. Do not continue the local from-scratch SAE basis.
2. M1912A passes exact BMP native cost and revises only the old raw-touch cost
   interpretation; it does not promote the weak frozen M1510 adapter.
3. M1911 is preserved as the strongest qrels-free latent milestone. Its 100M
   scale-up is scientifically authorized but remains a secondary branch due to
   native density and source-license requirements.
4. Preserve OpenSearch v2 plus M1660 BMP as the frozen product baseline.
5. Select the released Granite 30M Sparse checkpoint as the next model parent.
   Recover its NDCG/MAP/MRR deficit without losing fixed 192/50 activation
   budgets, Recall, index bytes, or latency.
6. Use the Apache Unified LSR framework as the auditable public-data training
   reference. It is not a second checkpoint parent.
7. Use OpenSearch doc v3 GTE only on held-out rows not named in its training
   provenance, and treat it as a product quality control rather than project
   progress.

The consolidated result and exact matrix are recorded in
`docs/research-sae/reports/m1900-m1999/ii42-m1900-m1913-learned-sparse-reset-consolidated-report.md`.

## Stop Rules

- Do not change a loss before its published schedule has actually completed.
- Do not scale a route whose complete-corpus quality or native engine gate
  fails.
- Do not promote a checkpoint evaluated on a dataset named in its training
  provenance.
- Do not call macro quality a breakthrough when row safety or product license
  fails.
- Do not tune TopK, thresholds, temperature, and FLOPS weights on BEIR rows.
- If both a properly scaled Latent Terms route and pretrained-SAE SPLARE route
  fail in the same BMP index, close SAE latent retrieval for this product and
  retain mature vocabulary sparse retrieval.

## Primary Sources

- SAE-SPLADE: <https://arxiv.org/abs/2604.21511>
- SPLARE: <https://arxiv.org/abs/2603.13277>
- Latent Terms: <https://arxiv.org/abs/2605.29384>
- CL-SR: <https://arxiv.org/abs/2506.00041>
- Embedding Scope: <https://arxiv.org/abs/2411.00786>
- SPLADE-v3: <https://arxiv.org/abs/2403.06789>
- DF-FLOPS: <https://arxiv.org/abs/2505.15070>
- Seismic: <https://arxiv.org/abs/2404.18812>
- BMP: <https://arxiv.org/abs/2405.01117>
- MLM-head rescaling: <https://arxiv.org/abs/2606.18811>
- Corpus-specific vocabulary: <https://arxiv.org/abs/2401.06703>
- Learned inference-free retrieval: <https://arxiv.org/abs/2505.01452>
- Vocabulary role: <https://arxiv.org/abs/2509.16621>
- Wacky Weights reproduction: <https://arxiv.org/abs/2605.19628>
- Unified LSR framework: <https://arxiv.org/abs/2303.13416>
- Unified LSR Apache-2.0 code: <https://github.com/thongnt99/learned-sparse-retrieval>
- Official SPLADE code and checkpoints: <https://github.com/naver/splade>
- Granite Embedding Models: <https://arxiv.org/abs/2502.20204>
- Granite 30M Sparse checkpoint: <https://huggingface.co/ibm-granite/granite-embedding-30m-sparse>
