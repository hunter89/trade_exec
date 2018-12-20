FROM tensorflow/tensorflow:1.11.0

RUN apt-get update \
    && apt-get install -y git python-tk

RUN git clone https://github.com/hunter89/tradeExecAgent.git

WORKDIR tradeExecAgent

RUN pip install -r requirements.txt