FROM python:3.12-slim-bookworm AS build
RUN apt-get update && apt-get install -y --no-install-recommends gcc libc6-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY 25B31026/calc4.c ./25B31026/calc4.c
RUN mkdir -p /app/bin && gcc -std=c11 -Wall -Wextra -Werror \
    25B31026/calc4.c -o /app/bin/calc4

FROM python:3.12-slim-bookworm
RUN apt-get update && apt-get install -y --no-install-recommends g++ bubblewrap \
    && rm -rf /var/lib/apt/lists/* \
    && groupadd --gid 10001 n4tive && useradd --uid 10001 --gid 10001 --no-create-home n4tive
WORKDIR /app
COPY --from=build /app/bin/calc4 ./bin/calc4
COPY 25B31026/codes/gas_to_cpp.py ./25B31026/codes/gas_to_cpp.py
COPY n4tive_site/ ./n4tive_site/
ENV HOST=0.0.0.0 PORT=8000 N4TIVE_COMPILER=/app/bin/calc4 \
    PYTHONDONTWRITEBYTECODE=1 PYTHONUNBUFFERED=1
USER 10001:10001
EXPOSE 8000
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s \
    CMD python -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8000/healthz', timeout=3)"
CMD ["python", "n4tive_site/server.py"]
