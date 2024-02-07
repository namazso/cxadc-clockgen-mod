# Scripts

Random scripts related to this project

## [capture-vhs.sh](capture-vhs.sh)

A simple bash script to record all 3 streams from a VHS unsing the clock generator.
- RF Video from a CXADC
- RF Audio from a CXADC
- Linear Audio

## [collect-info.sh](collect-info.sh)

A simple bash script to collect some system info to help trouble shooting.
Download to your linux box and run with:

```bash
bash collect-info.sh
```

It will create a *.tar.gz* that contains the relevant info.

If you want to, you can run one of the following commands to download and execute on the fly.
However be aware that you are blindly trusing the source url and script to [not be mallicious](https://0x46.net/thoughts/2019/04/27/piping-curl-to-shell/).
Even though *conveniant*, please review the script content before running this (by going to the used url in your browser and reading the script):

```bash
# if you have curl installed
curl https://gitlab.com/wolfre/cxadc-clock-generator-audio-adc/-/raw/main/scripts/collect-info.sh | bash
# if you have wget installed
wget -O - https://gitlab.com/wolfre/cxadc-clock-generator-audio-adc/-/raw/main/scripts/collect-info.sh | bash
```
 
