FROM ghcr.io/dragonminded/libdragon@sha256:9fe2843c9b7289393c1885be62391140e8b4cdb3edbc715f202dea16f554c5d2
ARG LIBDRAGON_REV=e356bf3f56f7afbf7e5246329562f145965cfdfc
RUN git clone https://github.com/DragonMinded/libdragon.git /libdragon \
    && git -C /libdragon checkout "$LIBDRAGON_REV" \
    && make -C /libdragon -j4 libdragon tools \
    && make -C /libdragon install tools-install
WORKDIR /work
