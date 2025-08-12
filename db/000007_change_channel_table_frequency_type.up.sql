ALTER TABLE public.channels
  DROP COLUMN frequency;

ALTER TABLE public.channels
  ADD COLUMN frequency integer NOT NULL;
