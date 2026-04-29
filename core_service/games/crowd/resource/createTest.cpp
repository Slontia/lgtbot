// [id] [type]
// [type] == 0 -> [players] [options] [for k = 0 to options: score[k] when all option k]

#include<bits/stdc++.h>

using namespace std;

int select[100],score[100];

void solveShort(string name,int id,int options,int players)
{
printf("GAME_TEST(%d, ",players);
cout<<name;
printf("){\n");
printf("    ASSERT_PUB_MSG(OK, 0, \"回合数 1\"); ASSERT_PUB_MSG(OK, 0, \"测试 %d\");",id);
printf(" StartGame();\n");
printf("        for(int i = 0; i < %d; i++) ASSERT_PRI_MSG(OK, i, \"%c\");\n",players-1,select[0]+'A');
printf("        ASSERT_PRI_MSG(CHECKOUT, %d, \"%c\");\n",players-1,select[0]+'A');
printf("        ASSERT_SCORE(");
for(int i=0;i<players;i++)
{
	if(i!=0) printf(",");
	printf("%d",score[i]);
}
printf(");}\n");
printf("\n");
	return;
}

void solve(string name,int id,int options,int players)
{
printf("GAME_TEST(%d, ",players);
cout<<name;
printf("){\n");
printf("    ASSERT_PUB_MSG(OK, 0, \"回合数 1\"); ASSERT_PUB_MSG(OK, 0, \"测试 %d\");",id);
printf(" StartGame();\n");
for(int i=0;i<players;i++)
{
	if(i!=players-1) printf("        ASSERT_PRI_MSG(OK, %d, \"%c\");\n",i,select[i]+'A');
	else printf("        ASSERT_PRI_MSG(CHECKOUT, %d, \"%c\");\n",i,select[i]+'A');
}
printf("        ASSERT_SCORE(");
for(int i=0;i<players;i++)
{
	if(i!=0) printf(",");
	printf("%d",score[i]);
}
printf(");}\n");
printf("\n");
	return;
}

int main()
{
	freopen("in.txt","r",stdin);
	freopen("out.txt","w",stdout);
	
	printf("// Copyright (c) 2018-present,  ZhengYang Xu <github.com/jeffxzy>. All rights reserved.\n");
	printf("//\n");
	printf("// This source code is licensed under LGPLv2 (found in the LICENSE file).\n\n");
	printf("#include \"game_framework/unittest_base.h\"\n\n");
	
	printf("namespace lgtbot {\n\n");
	printf("namespace game {\n\n");
	printf("namespace GAME_MODULE_NAME {\n\n");
	
	int id,type,options,players,i,j;
	while(scanf("%d",&id)!=EOF)
	{
		if (id == 37) {
			ifstream file("test_37.txt");
			if (file) for (string line; getline(file, line); ) cout<<line<<endl;
			continue;
		}
		scanf("%d",&type);
		scanf("%d %d",&players,&options);
		if(type==0)
		{
			for(i=0;i<options;i++)
			{
				int temp;
				scanf("%d",&temp);
				
				for(j=0;j<players;j++)
				{
					select[j]=i;
					score[j]=temp;
				}
				
				string x="Q";
				x+=to_string(id);
				x+="_";
				x+="All_";
				x+=char(i+'A');
				solveShort(x,id,options,players);
			}
		}
		else
		{
			for(i=0;i<players;i++)
				scanf("%d",&select[i]);
			for(i=0;i<players;i++)
				scanf("%d",&score[i]);
			string x="Q";
			x+=to_string(id);
			x+="_";
			x+="Basic_";
			for(i=0;i<players;i++)
				x+=char(select[i]+'A');
			solve(x,id,options,players);
		} 
	}
	
	printf("} // namespace GAME_MODULE_NAME\n\n");
	printf("} // namespace game\n\n");
	printf("} // gamespace lgtbot\n\n");
	
	printf("int main(int argc, char** argv)\n");
	printf("{\n");
	printf("    testing::InitGoogleTest(&argc, argv);\n");
	printf("    gflags::ParseCommandLineFlags(&argc, &argv, true);\n");
	printf("    return RUN_ALL_TESTS();\n");
	printf("}\n");
	printf("");
	
	return 0;
}
