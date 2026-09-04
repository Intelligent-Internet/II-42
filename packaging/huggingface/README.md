---
license: apache-2.0
language:
- en
base_model:
- ibm-granite/granite-embedding-30m-sparse
tags:
- ii42
- onnx
- sparse
- sparse-encoder
- information-retrieval
- postgresql
pipeline_tag: feature-extraction
---

# II-42 Model (Beta 1)

This repository distributes the frozen **P2.2 ABI-v2 model checkout** used by
[II-42](https://github.com/Intelligent-Internet/II-42), a PostgreSQL-native
BM25 engine with optional semantic postings in the same index.

This is the existing release model, published for reproducible source and
package builds. It is not a newly trained checkpoint, a standalone database,
or a generic Sentence Transformers model directory. Use the II-42 runtime and
its tokenizer, calibration, lexical vocabulary, and scoring contract together.

## Model And Runtime

The sparse route derives from IBM's approximately 30M-parameter
[Granite sparse encoder](https://huggingface.co/ibm-granite/granite-embedding-30m-sparse/tree/ad82b1fd09541c998c8d45045d601c51fdb8a9b7).
II-42 supplies separate document/query ONNX exports and combines semantic
atoms with exact lexical atoms in one sparse posting namespace. Document
completion can run in background workers; queries use the same frozen model
identity as the index.

| Contract | Value |
| --- | --- |
| Release label | Beta 1 |
| Bundle | `ii42-p2.2-nfcorpus-v2` |
| Model ID | `ii42_p2_p22_nfcorpus_v2_smoke` |
| Model API | `ii42_model_v1` |
| Runtime ABI | `ii42_p2_unified_text_atoms_v2` |
| Upstream revision | `ad82b1fd09541c998c8d45045d601c51fdb8a9b7` |
| Manifest SHA-256 | `419e3521eff91bdca149d7014dc71a5cd9538d6904854849056f4f327dd30364` |
| ZIP SHA-256 | `eae9bb2d03af12aea45b3de126c24db59abb81365edb77c4211d6a0717c79965` |
| Total atom dimensions | 79,787: 29,522 lexical + 50,265 semantic |
| Uncompressed checkout | 400,908,109 bytes; manifest + 13 artifacts |
| II-42 build dependency | ONNX Runtime 1.29.0, C API 29 |

The `_smoke` suffix is part of the frozen model ID, not a placeholder to edit.
Changing model artifacts or their contract requires a separately qualified
checkout and index rebuild. Downloading this identical checkout does not by
itself require rebuilding existing indexes.

## Files

```text
checkout/
  manifest.json
  encoder/
    semantic_document_compiler.onnx
    semantic_query_compiler.onnx
    semantic_runtime.json
    tokenizer.json
    tokenizer_config.json
    merges.txt
    vocab.json
  lexical/vocabulary.json
  sae/atom_space.json
  calibration/
    query_calibration_rms.f32
    query_calibration_stats.npz
    scoring_profile.json
  provenance/p2_contract.json
ii42-p2.2-nfcorpus-v2.zip
ii42-p2.2-nfcorpus-v2.zip.sha256
milestone-model.json
LICENSE
NOTICE
```

The ZIP contains exactly the same checkout under `ii42-p2.2-nfcorpus-v2/`.
The separate file tree makes the ONNX graphs and metadata inspectable; the
ZIP supports the existing II-42 package/CI downloader without introducing a
new loader or changing the model contract. `milestone-model.json` is the
source repository's content lock.

Model-card, license, Hub cache, and Git metadata are intentionally **outside**
`checkout/`: the release validator requires an exact model file inventory.
Historical local paths in the frozen provenance metadata record export
origins; they are not runtime dependencies or additional required downloads.

## Download And Build

Public downloads do not require a Hugging Face account or access token. From
an II-42 source checkout, install the official `hf` CLI if needed and run:

```bash
hf download Intelligent-Internet/II-42-Model-Beta-1 \
    ii42-p2.2-nfcorpus-v2.zip ii42-p2.2-nfcorpus-v2.zip.sha256 \
    --local-dir .artifacts/ii42-model-download
```

For repeatable builds, add `--revision` with the full Hub commit SHA from the
repository's Files and versions page. Use the ZIP, not the download directory
containing Hub cache metadata, as the input to the II-42 fetcher:

```bash
python3 scripts/fetch_milestone_model.py \
    --url "file://$PWD/.artifacts/ii42-model-download/ii42-p2.2-nfcorpus-v2.zip" \
    --archive-sha256 "$(awk '{print $1}' .artifacts/ii42-model-download/ii42-p2.2-nfcorpus-v2.zip.sha256)" \
    --output .artifacts/ii42-milestone-model
python3 scripts/validate_milestone_model_checkout.py \
    --checkout .artifacts/ii42-milestone-model
```

The fetcher checks the archive checksum and the source-controlled manifest,
all artifact digests, and exact inventory before installing the checkout. It
refuses to overwrite an existing output by default. See II-42's
[contributor guide](https://github.com/Intelligent-Internet/II-42/blob/main/CONTRIBUTING.md)
for ZIP/Docker builds and the pinned ONNX Runtime SDK.

Compiling the extension does not require these model weights. Complete
release ZIPs and Docker images do; source-installed SAE indexes also need an
administrator-installed checkout. Ordinary exact BM25 indexes do not perform
model inference.

## Evaluation Boundary And Direction

P2.2 is the full-text, lifecycle-qualified package successor to P2.1. The
project's historical P2.1 BEIR15/MTEB10 comparisons are not measurements of
this P2.2 checkout under every corpus, storage profile, or query workload.
See the project's technical reports through its
[documentation index](https://github.com/Intelligent-Internet/II-42/blob/main/docs/README.md)
for the evidence and reproduction boundaries.

The upstream encoder is English-focused. This checkout's lexical vocabulary
and query calibration are frozen from NFCorpus; domain transfer and other
languages require separate evaluation. The upstream model card documents
both public and non-public training sources; this release distributes the
derived runtime artifacts, not the upstream training corpus. Future work
focuses on broader corpus calibration, transfer quality, and the cost of
high-document-frequency semantic postings.

## License And Attribution

Distributed under [Apache License 2.0](https://huggingface.co/Intelligent-Internet/II-42-Model-Beta-1/blob/main/LICENSE).
The upstream model was developed by IBM's Granite Embedding Team. See
[NOTICE](https://huggingface.co/Intelligent-Internet/II-42-Model-Beta-1/blob/main/NOTICE) for the exact
upstream revision and II-42 modifications, and the
[upstream model card](https://huggingface.co/ibm-granite/granite-embedding-30m-sparse/blob/ad82b1fd09541c998c8d45045d601c51fdb8a9b7/README.md)
for its attribution, intended uses, and training-data description.
