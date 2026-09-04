# M1918 Public Retrieval Pilot Data Audit

Date: 2026-07-13

Decision: **pass the query-disjoint public-data gate and authorize the frozen
teacher observability canary. This report does not authorize model training by
itself.**

## Locked Source

- Dataset: `rlhn/rlhn-400K`
- Revision: `6286889b5d2c389e466a1b7bb922d28dbd2ade5a`
- Retained subsets: NQ, HotpotQA, FEVER
- Excluded subsets: FiQA, ArguAna, and MS MARCO
- Maximum hard negatives per row: 15
- Split: normalized query-text SHA-256, seed 1918, 90/10

The complete fixed-revision scan contained 390,175 rows. Of these, 219,372
belonged to excluded subsets. The remaining three subsets contained 170,803
eligible rows; strict normalized-query deduplication removed 554 and retained
170,249. This corrects the earlier 140,105-row estimate in the draft contract.

The RLHN repository describes the data as existing retrieval training sets
whose false hard negatives were relabeled. It is therefore useful for a
candidate-ordering pilot, but it is not evidence of a new independent corpus.

## Audited Surface

| Split | Queries | FEVER | HotpotQA | NQ | Bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| train | 153,251 | 25,488 | 75,962 | 51,801 | 1,801,468,811 |
| heldout | 16,998 | 2,837 | 8,427 | 5,734 | 199,511,065 |
| total | 170,249 | 28,325 | 84,389 | 57,535 | 2,000,979,876 |

Every row has at least one positive and one hard negative. Almost all rows
retain all 15 hard negatives; positive count has median 2 and p95 4.

## Strict Verification

The full remote audit reparsed both JSONL files and verified:

- manifest schema and fixed dataset revision;
- exact line count, byte size, and SHA-256 for both splits;
- normalized query text reproduces every stored query hash;
- deterministic split assignment reproduces every stored split;
- zero duplicate query hashes within or across splits;
- positive and negative document IDs are nonempty, unique, and disjoint;
- every row belongs to an allowed subset;
- combined subset counts equal the generation manifest.

Result: `passed`, with 170,249 unique query hashes and zero cross-split query
hash overlap.

## Limits

The split is query-disjoint, not passage-disjoint. That is appropriate for a
retrieval response pilot, but final generalization claims still require the
locked BEIR native surfaces. RLHN labels are used only for public heldout
selection and pairwise diagnostics; no BEIR qrels may select a checkpoint.

The next gate must retrieve from a shared passage universe with frozen PPLX,
OpenSearch, and M1914. Scoring only the 15 supplied hard negatives is
insufficient because it cannot measure parent-only candidate rescue.

## Artifacts

- `ii42-m1918-rlhn-public-pilot-manifest.json`
- `ii42-m1918-rlhn-public-pilot-audit.json`
- `scripts/prepare_m1918_rlhn_public_pilot.py`
- `scripts/audit_m1918_public_data.py`
- `scripts/run_m1918_prepare_rlhn_public_pilot_spark.sh`

Source: <https://huggingface.co/datasets/rlhn/rlhn-400K>
