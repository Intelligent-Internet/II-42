# M1640C Paper-Native Block16 Contract

## Trigger

The complete FiQA M1640B OpenSearch control preserved exact top100 and passed
the metadata-work gate, but decoded `0.699170` of all active postings. This
misses the locked `0.60` traversal gate while using block size 64.

The BMP paper (`2405.01117`) reports block size 16 as its best exact setting
for top100 retrieval. M1640C tests that single paper-native correction. It is
not a block-size search and cannot retroactively turn M1640B into a pass.

## Fixed Test

- Same Apache-2.0 OpenSearch checkpoint and revision as M1640B.
- Same cached complete FiQA query/document sparse vectors.
- Same document order, exact scores, top100, and qrels.
- Change only block size from 64 to 16.
- No query pruning, approximation, threshold, model change, or retraining.

## Gate

The correction is useful only if all conditions hold together:

- top100 parity is 1.0 for all 648 queries;
- decoded/full is at most 0.60;
- bitset word/full is at most 0.30;
- rankings are not underfilled.

If traversal improves but metadata exceeds its budget, stop rather than test
block 8/32 or tune compression assumptions. If traversal still fails, close
the contiguous fixed-block simulator for this representation.

## Interpretation

Failure does not show that learned sparse retrieval or one inverted index is
impossible. It shows that this project's uncompressed fixed-document-block
simulator is insufficient. A later engine route must reproduce actual BMP with
BP document ordering and compressed block-max metadata, or use another mature
native learned-sparse engine. Model training must not be used to hide that
engine gap.
