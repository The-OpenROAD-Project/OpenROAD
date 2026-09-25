from openroad import Tech, set_thread_count, thread_count

tech = Tech()
set_thread_count(1)
assert thread_count() == 1
