ALTER TABLE public.channels
  DROP COLUMN frequency;

ALTER TABLE public.channels
  ADD COLUMN frequency numeric(8, 4) NOT NULL;
