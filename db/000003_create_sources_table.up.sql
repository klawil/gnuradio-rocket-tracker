CREATE TABLE public.sources (
    id character varying(20) NOT NULL,
    created_at timestamp(6) without time zone DEFAULT (timezone('utc', now())),
    updated_at timestamp(6) without time zone DEFAULT (timezone('utc', now())),
    deleted_at timestamp(6) without time zone,
    source_type character varying(20) NOT NULL,
    source_source character varying(50)
);
ALTER TABLE public.sources OWNER TO william;
ALTER TABLE ONLY public.sources
    ADD CONSTRAINT sources_id PRIMARY KEY (id);
GRANT ALL ON TABLE public.sources TO rockettracker;
GRANT ALL ON TABLE public.sources TO william;
