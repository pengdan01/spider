rm -f test-linode/output/*
rsync -cavz --progress ../deploy_proxy m07:~/workspace
ssh m07 'cd workspace/deploy_proxy/test-linode; python test.py'
