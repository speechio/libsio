# libsio
A runtime library for Speech Input (stt) & Output (tts)

### Speech To Text
* unified CTC and WFST decoding via beam search
* online(streaming) decoding
* lattice-free on the fly rescoring with arbitrary language models, e.g.:
	```
	CTC + external LM
	WFST - (lookahead LM) + (big LM)
	E2E - (estimated internal LM) + (external LM)
	flexible contextual biasing
	```
* streaming + lattice free -> low latency
* modular design with potentials to deploy models from various speech toolkits such as Kaldi, K2, Espnet, SpeechBrain, WeNet.

### Text To Speech
Long term plan, won't happen soon.
### Non Goal
* GPU support

## Zen
* Keep it simple. All softwares are fated to fight against complexity.
* Keep it small. Codes are debts, less code means less interests to pay.
* Keep it explicit. Hidden behaviors behind non-obvious codes are evil.
* Keep it local. Things should be organized locally for easy understanding.
* Keep it concrete. Unnecessary abstractions bring cognitive costs, abstract only when it is absolutely necessary.

## License
TBD