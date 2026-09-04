# M1911 Nomic Latent Terms Reproduction Contract

Date: 2026-07-12

Status: **complete; see `docs/research-sae/reports/m1900-m1999/ii42-m1911-nomic-latent-terms-reproduction-report.md`**

## Question

M1910 proved that Latent Terms can be represented as one exact BMP inverted
index. It also showed that the old M1540 BGE SAE is not a quality milestone:
full official FiQA Recall@100 was `0.572944`, and removing the highest-DF 1%
of latent terms reduced it further. The remaining question is therefore about
the published representation, not the engine:

> Can a qrels-free TopK SAE trained on token states from the published Nomic
> dense retriever reproduce useful full-corpus retrieval and native BMP cost?

M1911 is a cleanly specified reproduction ladder for the Latent Terms recipe.
M1911B is a 10M-token mechanism pilot, not the paper's stated 30B-token run.
It must not reuse a project SAE checkpoint, BEIR labels, or a
retrieval-specific loss.

## Pinned Artifacts

- Dense backbone: `nomic-ai/nomic-embed-text-v1.5` at revision
  `e9b6763023c676ca8431644204f50c2b100d9aab`.
- Required custom model code: `nomic-ai/nomic-bert-2048` at revision
  `7710840340a098cfb869c4f65e87cf2b1b70caca`.
- Backbone license: Apache-2.0.
- Pilot corpus: `codelion/fineweb-edu-10M` at revision
  `6d4f3cb7cde82ce7abd8a8bdd51073e46caf81b4`.
- Pilot corpus shape: 9,457 documents and about 10M tokens, obtained through
  reservoir sampling from `HuggingFaceFW/fineweb-edu`.
- Source corpus revision: `87f09149ef4734204d70ed1d046ddc9ca3f2b8f9`.
- Source corpus license: ODC-BY.

The 10M derivative card does not declare its own license. Results may be used
for internal research, but release requires resolving that metadata gap or
rebuilding an equivalent sample directly from the pinned source corpus.

## Frozen Representation

- Input: final token hidden states from the frozen Nomic backbone.
- Prefixes: `search_document:` for FineWeb-Edu training text and indexed
  documents; `search_query:` for retrieval queries.
- SAE width: 32,768.
- Token sparsity: TopK-16.
- Objective: standard token reconstruction only.
- Pooling: sum token latent activations per text.
- Transform: square root after pooling.
- Retrieval score: BM25 over latent terms with `k1=8` and `b=0.7`.
- Native engine: the exact M1910 BMP compilation and quantization path.

No qrels, dataset ID, dense top-k target, candidate ranking loss, BM25 word
features, or project-specific posting teacher may enter SAE training.

High-DF latent terms are reported rather than removed. Unlike SAE-SPLADE's
dot-product score, Latent Terms uses corpus DF and BM25 IDF specifically to
handle a heavy Zipf-like head. M1910 already showed that pruning the highest-DF
1% reduced full FiQA Recall, so `prune1` remains a diagnostic surface and can
never select a checkpoint or rescue the full representation.

The Latent Terms paper does not state Nomic's task prefixes. The pinned Nomic
model card says a task prefix is mandatory, so this contract follows the
model's official retrieval interface rather than treating prefix choice as an
evaluation-tuned hyperparameter.

### Paper-Specified And Local Choices

Appendix A specifies TopK SAE, width 32,768, TopK-16 at train and inference,
transposed decoder initialization, Kaiming decoder initialization, AdamW,
peak learning rate `1e-3`, batch 4,096, 5% warmup plus cosine decay, three
epochs, five seeds, and FineWeb-Edu. M1911 locks all of those fields.

The paper does not specify AdamW weight decay, gradient clipping, AuxK size and
coefficient, dead-feature threshold, or the Nomic task prefixes. M1911 reports
these as local Gao-style implementation choices rather than paper facts:

- weight decay `0.0` and gradient norm cap `1.0`;
- AuxK `512`, coefficient `1/32`, dead threshold 10M token presentations;
- decoder bias initialized to the activation-cache mean and encoder bias to
  zero;
- official Nomic `search_document:` and `search_query:` prefixes.

These values are fixed before retrieval evaluation. A failed pilot cannot be
repaired by searching them on FiQA; a full paper claim would require author
code or clarification of the missing fields.

The pinned corpus shard contains `9,864,674` valid non-special activation
tokens under the required Nomic document prefix. The original `9.8M` train
plus `200k` validation quota was therefore impossible. M1911B locks `9.6M`
train tokens, `200k` validation tokens, and a deterministic `2.5%`
document-hash validation split. Before loading the GPU model, the activation
builder must run a tokenizer-only capacity audit over the same chunking and
split rules. This is a data-feasibility correction made without retrieval
labels, not a model or metric-driven hyperparameter change.

## Scale Ladder

### M1911A: Artifact and activation audit

1. Verify model and corpus checksums, the separately pinned custom model code,
   token-state shape, and prefix handling. A locally downloaded weight folder
   without the custom-code snapshot is an invalid artifact.
2. Measure actual token exposure, activation storage, and projected training
   memory before a long run.
3. Train a 1,000-step single-seed smoke only to verify reconstruction,
   TopK-16, checkpoint persistence, and ClearML tracking.

Stop on non-finite loss, dead-feature growth without recovery, an ambiguous
token-state interface, or an unauditable custom-code dependency.

### M1911B: Fixed 10M-token pilot

Train five fixed seeds on the complete 10M-token pilot using the paper-shaped
optimizer:

- AdamW;
- peak learning rate `1e-3`;
- 5% linear warmup followed by cosine decay;
- effective token batch target 4,096;
- three corpus epochs.

If hardware memory requires microbatching, use deterministic gradient
accumulation and report the exact effective batch. Do not alter width, TopK,
learning rate, epochs, or corpus in response to BEIR metrics.

The paper reports the mean of five seeds, so retrieval quality must be reported
as mean and standard deviation across all five. Select one representative seed
for the expensive native BMP closure before retrieval evaluation, using only
qrels-free diagnostics:

1. validation reconstruction error;
2. dead-feature fraction;
3. latent load balance and document-frequency distribution;
4. stability across held-out FineWeb-Edu documents.

### M1911C: Complete native closure

Encode the complete official FiQA corpus and queries with all five seeds and
report float Latent Terms quality as mean and standard deviation. For the
representative seed selected before retrieval evaluation, also report the
quantized BMP result, strict exact-top-k parity, index bytes, document/query
nnz, term DF distribution, and p50/p95 latency. The first retrieval evaluation
is also the final pilot evaluation; no FiQA-driven retraining or seed change is
permitted.

The selected-seed closure is routed by
`scripts/run_m1911_selected_seed_bmp_spark.sh`. The wrapper requires the
qrels-free training-selected seed to match the frozen five-seed FiQA summary,
verifies that the encoded surface used the selected checkpoint, and then
delegates both `full` and predeclared `prune1` surfaces to the already audited
M1910 exact-BMP runner. It adds no scoring or engine implementation.

The paper table states `30B` unique tokens, three epochs, and less than two
hours per run on one A100. Those claims are computationally inconsistent. This
pilot therefore reports two quantities separately: `9.8M` unique activation
tokens (`9.6M` train plus `0.2M` validation) and `28.8M` training-token
presentations. It does not relabel them as 30B.

### M1911D: Evidence-based scale-up

Scale to a directly sampled 100M-token source-corpus shard only if M1911C has
one of these outcomes:

- full FiQA Recall@100 is at least `0.62`; or
- it improves M1910 Recall@100 by at least `0.04` absolute while reconstruction
  and DF diagnostics still improve with exposure.

The 100M run keeps the same representation and optimizer. A larger run is not
authorized merely because training loss remains non-zero.

## Milestone Gates

M1911 becomes a learned-sparse milestone only if the complete official FiQA
surface satisfies all of the following:

1. Recall@100 is at least the OpenSearch v2 control value `0.656042`.
2. NDCG@10 and MAP@100 are no more than 5% relatively below the OpenSearch v2
   control values `0.370244` and `0.310860`.
3. Quantized BMP retains at least 99% of float Recall@100.
4. BMP has strict-boundary exactness and is faster at p95 than exhaustive
   sparse scoring.
5. Index bytes, latency, nnz, and DF are reported without a hidden pruning
   rescue.

Passing the pilot scale-up gate does not imply passing the milestone gate.

For context only, the paper reports FiQA NDCG@10 `0.377` for Nomic dense,
`0.382` for Nomic+Latent Terms, and `0.374` for SPLADE-v3. These are external
reference values, not local selection thresholds. M1911 keeps the locally
native-verified OpenSearch/BMP control as its gate and never selects a seed or
training setting from the paper-to-local metric gap.

## Stop Rules

- Do not use BEIR rows to select seeds, TopK, width, learning rate, or epochs.
- Do not claim the paper's stated token scale unless actual exposures match it.
- Do not add a retrieval loss before the reconstruction-only reproduction is
  closed.
- Stop this route if 100M tokens fail to improve the complete native surface
  enough to project a plausible OpenSearch-v2 crossing.
- If representation quality passes but native cost fails, diagnose the engine;
  if native cost passes but quality fails, diagnose the representation. Do not
  mix the two conclusions.

## Promotion Boundary

M1911 is a research milestone, not yet a product default. Promotion requires a
second untouched official dataset and a license-clean, reproducible source
sample. Only after that milestone may the project test lexical residuals,
retrieval distillation, or a smaller unified encoder.
