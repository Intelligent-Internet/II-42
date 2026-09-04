# M1912 Pretrained-SAE SPLARE Artifact Audit

Date: 2026-07-12

Status: **artifact gate passed; training not yet authorized**

## Purpose

SPLARE is the strongest published evidence that a mature pretrained SAE can
replace a vocabulary projection inside a trained sparse retriever. The paper
does not publish a SPLARE checkpoint or an executable training repository.
M1912 therefore separates two questions:

1. Are the exact backbone/SAE building blocks identifiable, downloadable, and
   licensed?
2. Can II-42 independently reproduce the paper-shaped training without
   silently substituting another architecture or tuning on target BEIR rows?

Passing the first question does not claim that the second has been completed.

## Prior II-42 Evidence

M1510 already evaluated an independent Gemma-2-2B/Gemma-Scope LoRA adapter,
not an official SPLARE checkpoint. It produced genuine retrieval signal: it
beat dense and P1 on NFCorpus and beat P1 on SciFact, but it was substantially
weaker than dense and P1 on FiQA. M1518 then continued that adapter with
ordinary FLOPS and DF-FLOPS for only 400 optimizer steps; neither removed its
universal-support pattern.

Those experiments do not reproduce the paper route. They use layer 18/L0 116,
an external multilingual adapter, no audited 10,000-step masked-next-token
stage, and a bounded continuation rather than the paper's full English
MS MARCO training. They do, however, prove that the mechanism can learn useful
text-derived sparse signal and that more threshold/FLOPS tuning of the same
adapter is closed.

The M1510 deployment stop also relied on raw posting-union touch. M1910 later
showed that this proxy can disagree with measured BMP size and latency.
M1912A therefore replays the frozen M1510 FiQA postings through exact BMP before
any 2B training is considered. A native-cost pass can revise the engine-cost
claim, but cannot repair M1510's FiQA quality or provenance limitations.

## Audited Artifact Options

### Gemma Scope mechanism route

- Backbone: `google/gemma-2-2b` at
  `c5ebcd40d208330abc697524c919956e692655cf`.
- Backbone access: public metadata, manual Hugging Face gate.
- Backbone license: Gemma terms.
- SAE repository: `google/gemma-scope-2b-pt-res` at
  `fd571b47c1c64851e9b1989792367b9babb4af63`.
- SAE license: CC-BY-4.0.
- Paper-shaped first choice:
  `layer_16/width_65k/average_l0_128/params.npz`.
- SAE payload size: `1,208,494,296` bytes.
- SAE payload SHA-256:
  `b9e5d73bdb10ae8eb48a949886b9716c23a779cbc35de6dbc110e4ee72de1012`.

Layer 16 matches the paper's strongest roughly two-thirds-depth Gemma-2-2B
region. Among the published layer-16/65k choices, average L0 128 is the closest
available value to the paper's stated preference for L0 near 100.

This is the preferred first executable mechanism route because it is much
smaller than Llama-3.1-8B while preserving the paper's defining ingredients:
pretrained residual SAE, frozen SAE, intermediate layer, LoRA backbone update,
SPLADE pooling, listwise distillation, and FLOPS.

### Llama Scope main-result route

- Backbone: `meta-llama/Llama-3.1-8B` at
  `d04e592bb4f6aa9cfee91e2e20afa771667e1d4b`.
- Backbone access: manual Hugging Face gate.
- Backbone license: Llama 3.1 Community License.
- SAE repository: `OpenMOSS-Team/Llama3_1-8B-Base-LXR-32x` at
  `336730e758fed3cb2273276703d836aa8659d293`.
- SAE front-page license: Apache-2.0.
- Main paper choice:
  `Llama3_1-8B-Base-L26R-32x/checkpoints/final.safetensors`.
- SAE width: 131,072; residual stream after layer 26.
- SAE payload size: `2,147,754,376` bytes.
- SAE payload SHA-256:
  `de874b38bfd2380675ef358b6ba2513d20568e6d050190c41cec56a9dbab1880`.

This route is closest to the paper's reported English SPLARE result, but it is
not the first implementation target. It has higher storage, memory, inference,
and license-gate cost, and should be attempted only after the 2B mechanism
implementation reproduces the expected training behavior.

## Required Training Shape

An honest SPLARE reproduction is not merely loading a pretrained SAE and
running BM25. It requires all of the following:

1. expose an intermediate residual stream and enable bidirectional attention;
2. perform 10,000 steps of 20%-masked next-token pretraining on MS MARCO;
3. freeze the residual SAE and train rank-64 LoRA adapters in the backbone;
4. train one epoch with batch 128, eight negatives, learning rate `5e-5`, 1%
   warmup, cross-encoder KL, and query/document FLOPS `1e-4`;
5. use paper-fixed temperature `50` for Gemma Scope or `80` for Llama Scope;
6. apply inference TopK `(40, 400)` and report the pre-TopK sequence sparsity;
7. run complete native inverted-index quality and cost closure.

The paper selected temperatures with NanoBEIR nDCG. II-42 may use the published
fixed temperatures for reproduction, but must not re-grid them on FiQA or
other target BEIR rows.

## Missing Pieces

- No official SPLARE checkpoint has been located.
- No official SPLARE training source has been located.
- The exact English hard-negative and DeBERTa-v3 teacher artifacts still need
  pinned revisions and checksums.
- Backbone access must be verified on the selected remote training node before
  any large download.
- The paper's Gemma route is primarily an ablation surface; the main published
  English result uses the larger Llama route.

## Execution Gate

M1912 training is authorized only after M1904/M1905, M1911, and the frozen
M1912A native-cost re-audit have closed.
Before training, a separate contract must pin the English data/teacher,
backbone access, SAE loader, ClearML task, memory estimate, and untouched
evaluation rows.

The first run must be a Gemma-2-2B forward/pooling audit, followed by a
short masked-pretraining smoke and a short listwise smoke. It may scale only if
the frozen SAE remains active, sequence L0 moves toward the paper's range, and
teacher agreement improves without universal-feature collapse.

## Sources

- SPLARE: <https://arxiv.org/abs/2603.13277>
- Llama Scope: <https://arxiv.org/abs/2410.20526>
- Llama Scope artifacts: <https://huggingface.co/OpenMOSS-Team/Llama-Scope>
- Gemma Scope artifacts:
  <https://huggingface.co/google/gemma-scope-2b-pt-res>
