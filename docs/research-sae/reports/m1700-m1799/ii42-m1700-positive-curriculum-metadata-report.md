# M1700 Positive Curriculum Metadata Report

## Decision

**Downloaded artifacts and deterministic manifests are verified. Authorize the
fixed batch-128 resource canary; training is not yet authorized.**

The live Hugging Face metadata audit resolved every dataset artifact by pinned
revision and verified repository identity, card schema, row count, Parquet
shard count, compressed byte total, and LFS object metadata.

| Surface | Revision | Rows | Bytes | Shards | Result |
| --- | --- | ---: | ---: | ---: | --- |
| Broad weak pairs | `dfea387946f33796beee9b71caef61e091cfa2bf` | 9,259,451 | 13,478,134,157 | 46 | pass |
| Curated hard negatives | `d5323fad7aea636e633be28722ad15aafa6cbd54` | 648,766 | 6,447,294,919 | 11 | pass |

All required fields are present. The broad surface contains query IDs, query
text, positive passages, and an empty negative-passage field. RLHN additionally
contains curated negative passages and source subset provenance.

## Download Verification

Both pinned repositories were downloaded to `/Volumes/Betty/II42/m1700`.
Every Parquet file was hashed locally and compared with its Hugging Face LFS
object ID. The pinned README files were also compared byte-for-byte.

| Surface | Verified files | Verified bytes | SHA/LFS result |
| --- | ---: | ---: | --- |
| Broad weak pairs | 46/46 | 13,478,134,157 | exact |
| Curated hard negatives | 11/11 | 6,447,294,919 | exact |

Decision: `authorize_s0_manifest_and_leakage_audit`.

## Manifest And Leakage Audit

The manifest builder scanned all 9,259,451 broad-pair rows rather than taking a
contiguous prefix. It normalized text with NFKC, case folding, and whitespace
collapse, then selected the globally lowest deterministic query hashes.

- unique query hashes: `9,124,265`;
- duplicate query hashes removed: `135,186`;
- locked shared15 evaluation query hashes: `1,342`;
- exact training-query overlaps: `0`;
- positive-text/evaluation-query overlaps: `0`;
- missing query or positive rows: `0`;
- selected train/validation rows: `100,000 / 10,000`;
- all 46 source shards represented: yes;
- train/validation query-hash intersection: empty.

| Manifest | Bytes | SHA-256 |
| --- | ---: | --- |
| train | 113,647,246 | `b8de873998eeb363cceb42d8e1cf858a6a93acfdcb92b702e347a2a550027de1` |
| validation | 11,788,547 | `1f98484284370bd104ac6cf6416b8157e7b39ad2c99c1a6e09bfda3554b75821` |

Decision: `authorize_s0_resource_canary`.

## Remaining S0 Gates

Before any training:

1. retain RLHN subset provenance and initially admit only
   `msmarco_passage`;
2. run the fixed batch-128 forward/backward resource canary.

The metadata pass does not imply model quality, absence of evaluation leakage,
or resource feasibility. Those remain explicit stop gates.

## Reproduction

```bash
python3 scripts/audit_m1700_positive_curriculum_metadata.py \
    --output-dir /tmp/ii42-m1700-metadata-audit-v2

python3 scripts/verify_m1700_dataset_downloads.py \
    --data-root /Volumes/Betty/II42/m1700/datasets \
    --output-dir /Volumes/Betty/II42/m1700/verification/downloads-v1

uv run --with pyarrow python \
    scripts/materialize_m1700_positive_pair_manifests.py \
    --data-root /Volumes/Betty/II42/m1700/datasets/\
nomic-embed-pretrain-lite-dfea3879 \
    --evaluation-root /Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared \
    --output-dir /Volumes/Betty/II42/m1700/manifests/\
positive-pairs-100k-v1
```

Tests: `8 passed` across metadata, download verification, and manifest suites.
