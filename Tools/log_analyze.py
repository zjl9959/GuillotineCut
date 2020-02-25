#!/usr/bin/python3

import json
import csv
import git
import datetime
import matplotlib

repo = git.Repo("../")
log = repo.commit('d99455')
date = datetime.datetime.fromtimestamp(log.authored_date)
print(date)
